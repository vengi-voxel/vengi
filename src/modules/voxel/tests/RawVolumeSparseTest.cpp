/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Thread.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/VolumeSamplerDispatch.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeVisitor.h"
#include <limits.h>

namespace voxel {

class RawVolumeSparseTest : public app::AbstractTest {
private:
	int _prevThreshold = 256;

protected:
	void SetUp() override {
		app::AbstractTest::SetUp();
		_prevThreshold = RawVolume::chunkedStorageThreshold();
	}

	void TearDown() override {
		RawVolume::setChunkedStorageThreshold(_prevThreshold);
		app::AbstractTest::TearDown();
	}
};

TEST_F(RawVolumeSparseTest, EmptyLargeVolumeMemoryFootprint) {
	RawVolume vol(Region(0, 0, 0, 511, 511, 511));
	ASSERT_TRUE(vol.valid());
	EXPECT_TRUE(vol.isSparse());
	EXPECT_FALSE(vol.isContiguous());
	EXPECT_EQ(vol.linearVoxels(), nullptr);
	// 512^3 dense would be 512 MiB; SparseChunk grid is 2x2x2 slots (< 4 KiB).
	EXPECT_LT(vol.allocatedBytes(), 4096u);
	EXPECT_TRUE(vol.isEmpty(vol.region()));
	EXPECT_TRUE(isAir(vol.voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(isAir(vol.voxel(511, 511, 511).getMaterial()));
}

TEST_F(RawVolumeSparseTest, ScatteredVoxelsStayPackedAndCompact) {
	RawVolume vol(Region(-256, -256, -256, 255, 255, 255));
	ASSERT_TRUE(vol.isSparse());
	const Voxel v1 = createVoxel(VoxelType::Generic, 42);
	const Voxel v2 = createVoxel(VoxelType::Generic, 99, 5);
	// Place 512 scattered voxels across the 512^3 volume (1 per 32^3 region)
	int count = 0;
	for (int z = -256; z < 256; z += 64) {
		for (int y = -256; y < 256; y += 64) {
			for (int x = -256; x < 256; x += 64) {
				EXPECT_TRUE(vol.setVoxel(x, y, z, (count & 1) ? v1 : v2));
				++count;
			}
		}
	}
	EXPECT_EQ(count, 512);
	// Even with 512 scattered voxels across 8 256^3 chunks, memory is ~600 KiB (vs 512 MiB dense, ~64 MiB for 32^3 chunks)
	EXPECT_LT(vol.allocatedBytes(), 2u * 1024u * 1024u);
	EXPECT_FALSE(vol.isEmpty(vol.region()));

	int idx = 0;
	for (int z = -256; z < 256; z += 64) {
		for (int y = -256; y < 256; y += 64) {
			for (int x = -256; x < 256; x += 64) {
				EXPECT_EQ(vol.voxel(x, y, z), (idx & 1) ? v1 : v2);
				EXPECT_EQ(vol.voxel(x + 1, y, z), Voxel());
				++idx;
			}
		}
	}

	// Erasing all voxels releases chunk memory back to minimal directory size
	for (int z = -256; z < 256; z += 64) {
		for (int y = -256; y < 256; y += 64) {
			for (int x = -256; x < 256; x += 64) {
				Voxel erase;
				erase.setNormalReset();
				EXPECT_TRUE(vol.setVoxel(x, y, z, erase));
				EXPECT_TRUE(vol.setVoxel(x, y, z, Voxel()));
			}
		}
	}
	EXPECT_TRUE(vol.isEmpty(vol.region()));
	EXPECT_LT(vol.allocatedBytes(), 4096u);
}

TEST_F(RawVolumeSparseTest, HollowShellMemoryAndSolidBounds) {
	RawVolume vol(Region(-128, -128, -128, 127, 127, 127));
	ASSERT_TRUE(vol.isSparse());
	const Voxel solid = createVoxel(VoxelType::Generic, 7);
	for (int y = -100; y <= 100; ++y) {
		vol.setVoxelsRow(-100, y, -100, 201, solid);
		vol.setVoxelsRow(-100, y, 100, 201, solid);
	}
	// Two 201x201 faces = 80,802 voxels in a 64 MiB volume: SparseChunk packs 64-voxel face bricks!
	EXPECT_LT(vol.allocatedBytes(), 4u * 1024u * 1024u);

	glm::ivec3 mins, maxs;
	ASSERT_TRUE(voxelutil::computeSolidBounds(&vol, mins, maxs));
	EXPECT_EQ(mins, glm::ivec3(-100, -100, -100));
	EXPECT_EQ(maxs, glm::ivec3(100, 100, 100));
	EXPECT_TRUE(vol.isSparse());
}

TEST_F(RawVolumeSparseTest, PackedToDensePromotionAndFillUniform) {
	RawVolume::setChunkedStorageThreshold(16);
	RawVolume vol(Region(0, 0, 0, 31, 31, 31));
	ASSERT_TRUE(vol.isSparse());

	// Fill an entire 8^3 brick (> PackedMaxVoxels = 64) to trigger dense promotion
	for (int z = 0; z < 8; ++z) {
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				vol.setVoxel(x, y, z, createVoxel(VoxelType::Generic, (uint8_t)((x + y * 8 + z * 64) & 0xFF)));
			}
		}
	}
	for (int z = 0; z < 8; ++z) {
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				EXPECT_EQ(vol.voxel(x, y, z).getColor(), (uint8_t)((x + y * 8 + z * 64) & 0xFF));
			}
		}
	}

	// Uniform fill releases all bricks and keeps memory tiny
	const Voxel filled = createVoxel(VoxelType::Generic, 13);
	vol.fill(filled);
	EXPECT_LT(vol.allocatedBytes(), 1024u);
	EXPECT_EQ(vol.voxel(0, 0, 0), filled);
	EXPECT_EQ(vol.voxel(31, 31, 31), filled);

	// Carve a single cell inside uniform-filled volume
	EXPECT_TRUE(vol.setVoxel(5, 6, 7, Voxel()));
	EXPECT_EQ(vol.voxel(5, 6, 7), Voxel());
	EXPECT_EQ(vol.voxel(5, 6, 8), filled);

	// Restore cell to default value releases brick back
	EXPECT_TRUE(vol.setVoxel(5, 6, 7, filled));
	EXPECT_EQ(vol.voxel(5, 6, 7), filled);
}

TEST_F(RawVolumeSparseTest, NormalMergeAndAirMetadataSemantics) {
	RawVolume::setChunkedStorageThreshold(8);
	RawVolume sparse(Region(0, 0, 0, 15, 15, 15));
	RawVolume::setChunkedStorageThreshold(256);
	RawVolume dense(Region(0, 0, 0, 15, 15, 15));
	ASSERT_TRUE(sparse.isSparse());
	ASSERT_TRUE(dense.isContiguous());

	const Voxel withNormal = createVoxel(VoxelType::Generic, 5, 12);
	EXPECT_EQ(sparse.setVoxel(3, 4, 5, withNormal), dense.setVoxel(3, 4, 5, withNormal));
	// Assigning default air preserves normalIndex per Voxel::operator=
	EXPECT_EQ(sparse.setVoxel(3, 4, 5, Voxel()), dense.setVoxel(3, 4, 5, Voxel()));
	EXPECT_EQ(sparse.voxel(3, 4, 5), dense.voxel(3, 4, 5));
	EXPECT_EQ(sparse.voxel(3, 4, 5).getNormal(), 12);
	EXPECT_TRUE(sparse.equals(dense));
	EXPECT_TRUE(dense.equals(sparse));

	// Flagged air cell
	sparse.setFlags(Region(1, 1, 1, 2, 2, 2), FlagOutline);
	dense.setFlags(Region(1, 1, 1, 2, 2, 2), FlagOutline);
	EXPECT_TRUE(sparse.equals(dense));
	EXPECT_EQ(sparse.regionForFlag(FlagOutline), dense.regionForFlag(FlagOutline));
	EXPECT_TRUE(sparse.hasFlags(Region(1, 1, 1, 2, 2, 2), FlagOutline));
	sparse.toggleFlags(Region(1, 1, 1, 2, 2, 2), FlagOutline);
	dense.toggleFlags(Region(1, 1, 1, 2, 2, 2), FlagOutline);
	EXPECT_FALSE(sparse.hasFlags(Region(1, 1, 1, 2, 2, 2), FlagOutline));
	EXPECT_TRUE(sparse.equals(dense));
}

TEST_F(RawVolumeSparseTest, SamplerTraversalAndPeeksAcrossBrickAndChunkBoundaries) {
	RawVolume::setChunkedStorageThreshold(16);
	// Span across 256 boundary (2 chunks) and multiple 8^3 bricks
	RawVolume vol(Region(-10, -10, -10, 260, 20, 20));
	ASSERT_TRUE(vol.isSparse());
	const Voxel border = createVoxel(VoxelType::Generic, 200);
	vol.setBorderValue(border);

	for (int x = -10; x <= 260; x += 3) {
		vol.setVoxel(x, 0, 0, createVoxel(VoxelType::Generic, (uint8_t)(x & 0x7F) + 1));
	}
	// Dense brick around x=240..247
	for (int z = -2; z <= 5; ++z) {
		for (int y = -2; y <= 5; ++y) {
			for (int x = 240; x <= 247; ++x) {
				vol.setVoxel(x, y, z, createVoxel(VoxelType::Generic, (uint8_t)((x + y + z) & 0x7F) + 1));
			}
		}
	}

	RawVolume::Sampler s(vol);
	ASSERT_TRUE(s.setPosition(-10, 0, 0));
	for (int x = -10; x <= 260; ++x) {
		EXPECT_EQ(s.voxel(), vol.voxel(x, 0, 0)) << "at x=" << x;
		EXPECT_EQ(s.peekVoxel1nx0py0pz(), vol.voxel(x - 1, 0, 0)) << "peek -x at x=" << x;
		EXPECT_EQ(s.peekVoxel1px0py0pz(), vol.voxel(x + 1, 0, 0)) << "peek +x at x=" << x;
		EXPECT_EQ(s.peekVoxel0px1py0pz(), vol.voxel(x, 1, 0)) << "peek +y at x=" << x;
		EXPECT_EQ(s.peekVoxel0px0py1pz(), vol.voxel(x, 0, 1)) << "peek +z at x=" << x;
		s.movePositiveX();
	}
	EXPECT_FALSE(s.currentPositionValid());
	EXPECT_EQ(s.voxel(), border);
}

TEST_F(RawVolumeSparseTest, MoveCopyAndMaterialize) {
	RawVolume::setChunkedStorageThreshold(16);
	RawVolume sparse(Region(-8, -8, -8, 23, 23, 23));
	RawVolume::setChunkedStorageThreshold(256);
	RawVolume dense(Region(-8, -8, -8, 23, 23, 23));
	ASSERT_TRUE(sparse.isSparse());
	ASSERT_TRUE(dense.isContiguous());

	for (int i = -8; i <= 23; ++i) {
		const Voxel v = createVoxel(VoxelType::Generic, (uint8_t)(i + 20));
		sparse.setVoxel(i, (i * 3) & 15, -8 + ((i * 5) & 31), v);
		dense.setVoxel(i, (i * 3) & 15, -8 + ((i * 5) & 31), v);
	}
	EXPECT_TRUE(sparse.equals(dense));

	// Copy constructor preserves sparsity
	RawVolume sparseCopy(sparse);
	EXPECT_TRUE(sparseCopy.isSparse());
	EXPECT_TRUE(sparseCopy.equals(dense));

	// Cyclic move matches dense exactly
	ASSERT_TRUE(sparse.move({5, -3, 11}));
	ASSERT_TRUE(dense.move({5, -3, 11}));
	EXPECT_TRUE(sparse.isSparse());
	EXPECT_TRUE(sparse.equals(dense));

	// Materialize via voxels() switches cleanly to dense with identical data
	Voxel *raw = sparse.voxels();
	ASSERT_NE(raw, nullptr);
	EXPECT_TRUE(sparse.isContiguous());
	EXPECT_FALSE(sparse.isSparse());
	EXPECT_TRUE(sparse.equals(dense));
}

TEST_F(RawVolumeSparseTest, CoordinateLimitsAndConcurrentWritesReads) {
	RawVolume::setChunkedStorageThreshold(2);
	for (int origin : {INT_MIN, 0, INT_MAX - 2}) {
		RawVolume vol(Region(origin, origin + 2));
		ASSERT_TRUE(vol.isSparse());
		const Voxel v = createVoxel(VoxelType::Generic, 11);
		vol.fill(v);
		EXPECT_EQ(vol.voxel(origin, origin, origin), v);
		EXPECT_EQ(vol.voxel(origin + 2, origin + 2, origin + 2), v);
		vol.setFlags(Region(origin + 1, origin + 2), FlagOutline);
		EXPECT_EQ(vol.regionForFlag(FlagOutline), Region(origin + 1, origin + 2));
		vol.clear();
		EXPECT_TRUE(vol.isEmpty(vol.region()));
	}

	// Concurrent writes across threads + concurrent reads
	RawVolume::setChunkedStorageThreshold(16);
	RawVolume vol(Region(0, 0, 0, 63, 63, 63));
	ASSERT_TRUE(vol.isSparse());
	core::Thread writers[4];
	for (int t = 0; t < 4; ++t) {
		writers[t] = core::Thread(
			[&vol, t]() {
				const Voxel v = createVoxel(VoxelType::Generic, (uint8_t)(t + 1));
				for (int z = t * 16; z < (t + 1) * 16; ++z) {
					for (int y = 0; y < 64; y += 2) {
						vol.setVoxel(y, y, z, v);
					}
				}
			},
			"sparse-writer");
	}
	for (auto &th : writers) {
		th.join();
	}

	core::AtomicInt mismatches{0};
	core::Thread readers[4];
	for (int t = 0; t < 4; ++t) {
		readers[t] = core::Thread(
			[&vol, &mismatches]() {
				RawVolume::Sampler s(vol);
				for (int z = 0; z < 64; ++z) {
					const uint8_t expectedColor = (uint8_t)(z / 16 + 1);
					for (int y = 0; y < 64; y += 2) {
						s.setPosition(y, y, z);
						if (s.voxel().getColor() != expectedColor) {
							mismatches.increment(1);
						}
					}
				}
			},
			"sparse-reader");
	}
	for (auto &th : readers) {
		th.join();
	}
	EXPECT_EQ((int)mismatches, 0);
}

} // namespace voxel
