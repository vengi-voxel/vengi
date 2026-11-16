/**
 * @file
 */

#include "voxel/ConcurrentSparseVolume.h"
#include "app/tests/AbstractTest.h"
#include "core/collection/Vector.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/tests/TestHelper.h"
#include <thread>
#include <vector>

namespace voxel {

class ConcurrentSparseVolumeTest : public app::AbstractTest {};

TEST_F(ConcurrentSparseVolumeTest, testSetVoxel) {
	voxel::ConcurrentSparseVolume v(voxel::Region(0, 10));
	ASSERT_EQ(0u, v.size());
	ASSERT_TRUE(v.empty());
	ASSERT_TRUE(v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(1u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_TRUE(v.setVoxel(10, 10, 10, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(2u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_FALSE(v.setVoxel(11, 11, 11, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(2u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_TRUE(v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_EQ(1u, v.size());
	ASSERT_FALSE(v.empty());
}

TEST_F(ConcurrentSparseVolumeTest, testCopyToRawVolume) {
	const voxel::Region region(0, 30);
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 0);
	voxel::ConcurrentSparseVolume v(region);
	voxel::RawVolume rv(region);
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				ASSERT_TRUE(v.setVoxel(x, y, z, voxel));
			}
		}
	}
	voxel::RawVolumeWrapper rvw(&rv);
	v.copyTo(rvw);
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				ASSERT_TRUE(voxel.isSameType(rv.voxel(x, y, z)));
			}
		}
	}
}

TEST_F(ConcurrentSparseVolumeTest, testSetVoxels) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 7> voxels;
	voxels.assign(voxel, voxels.capacity());
	voxel::ConcurrentSparseVolume v{voxel::Region{0, 0, 0, 3, 6, 3}};
	voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				v.region().getHeightInVoxels());
	const int vxls = voxelutil::countVoxels(v);
	ASSERT_EQ(v.region().voxels(), vxls);
}

TEST_F(ConcurrentSparseVolumeTest, testFullSamplerLoop) {
	const voxel::Region &region = voxel::Region::fromSize(glm::ivec3(64));
	ConcurrentSparseVolume v(region);
	v.setVoxel(1, 2, 1, voxel::createVoxel(VoxelType::Generic, 0));

	v.setVoxel(0, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(1, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(2, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(0, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(1, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(2, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(0, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(1, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(2, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));

	v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 1));
	v.setVoxel(1, 0, 0, voxel::createVoxel(VoxelType::Generic, 2));
	v.setVoxel(2, 0, 0, voxel::createVoxel(VoxelType::Generic, 3));
	v.setVoxel(0, 0, 1, voxel::createVoxel(VoxelType::Generic, 4));
	v.setVoxel(1, 0, 1, voxel::createVoxel(VoxelType::Generic, 5));
	v.setVoxel(2, 0, 1, voxel::createVoxel(VoxelType::Generic, 6));
	v.setVoxel(0, 0, 2, voxel::createVoxel(VoxelType::Generic, 7));
	v.setVoxel(1, 0, 2, voxel::createVoxel(VoxelType::Generic, 8));
	v.setVoxel(2, 0, 2, voxel::createVoxel(VoxelType::Generic, 9));
	ConcurrentSparseVolume::Sampler volumeSampler(v);

	ASSERT_EQ(0, region.getLowerX());
	ASSERT_EQ(0, region.getLowerY());
	ASSERT_EQ(0, region.getLowerZ());

	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z++) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y++) {
			volumeSampler.setPosition(region.getLowerX(), y, z);

			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x++) {
				const Voxel voxelCurrent = volumeSampler.voxel();
				const Voxel voxelLeft = volumeSampler.peekVoxel1nx0py0pz();
				const Voxel voxelRight = volumeSampler.peekVoxel1px0py0pz();
				const Voxel voxelBefore = volumeSampler.peekVoxel0px0py1nz();
				const Voxel voxelBehind = volumeSampler.peekVoxel0px0py1pz();
				const Voxel voxelLeftBefore = volumeSampler.peekVoxel1nx0py1nz();
				const Voxel voxelRightBefore = volumeSampler.peekVoxel1px0py1nz();
				const Voxel voxelLeftBehind = volumeSampler.peekVoxel1nx0py1pz();
				const Voxel voxelRightBehind = volumeSampler.peekVoxel1px0py1pz();

				const Voxel voxelAbove = volumeSampler.peekVoxel0px1py0pz();
				const Voxel voxelAboveLeft = volumeSampler.peekVoxel1nx1py0pz();
				const Voxel voxelAboveRight = volumeSampler.peekVoxel1px1py0pz();
				const Voxel voxelAboveBefore = volumeSampler.peekVoxel0px1py1nz();
				const Voxel voxelAboveBehind = volumeSampler.peekVoxel0px1py1pz();
				const Voxel voxelAboveLeftBefore = volumeSampler.peekVoxel1nx1py1nz();
				const Voxel voxelAboveRightBefore = volumeSampler.peekVoxel1px1py1nz();
				const Voxel voxelAboveLeftBehind = volumeSampler.peekVoxel1nx1py1pz();
				const Voxel voxelAboveRightBehind = volumeSampler.peekVoxel1px1py1pz();

				const Voxel voxelBelow = volumeSampler.peekVoxel0px1ny0pz();

				if (y == 0) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 1) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 2) {
					// 25
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelCurrent.getMaterial())
							<< "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
					}
				}

				volumeSampler.movePositiveX();
			}
		}
	}
}

TEST_F(ConcurrentSparseVolumeTest, testChunkBoundariesAndRegion) {
	ConcurrentSparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 2);
	ASSERT_TRUE(v.setVoxel(255, 255, 255, voxel));
	ASSERT_TRUE(v.setVoxel(256, 0, 0, voxel));
	ASSERT_TRUE(v.setVoxel(-1, -1, -1, voxel));

	EXPECT_EQ(3u, v.size());
	EXPECT_TRUE(v.hasVoxel(255, 255, 255));
	EXPECT_TRUE(v.hasVoxel(256, 0, 0));
	EXPECT_TRUE(v.hasVoxel(-1, -1, -1));

	const Region region = v.calculateRegion();
	ASSERT_TRUE(region.isValid());
	EXPECT_EQ(-1, region.getLowerX());
	EXPECT_EQ(-1, region.getLowerY());
	EXPECT_EQ(-1, region.getLowerZ());
	EXPECT_EQ(256, region.getUpperX());
	EXPECT_EQ(255, region.getUpperY());
	EXPECT_EQ(255, region.getUpperZ());
}

TEST_F(ConcurrentSparseVolumeTest, testThreadSafeChunkedSetVoxel) {
	ConcurrentSparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 3);
	const int threadCount = 4;
	const int voxelsPerThread = 64;
	std::vector<std::thread> threads;
	threads.reserve(threadCount);

	for (int t = 0; t < threadCount; ++t) {
		threads.emplace_back([&, t]() {
			for (int i = 0; i < voxelsPerThread; ++i) {
				const int z = (t % 2 == 0) ? i : 256 + i;
				v.setVoxel(i, t, z, voxel);
			}
		});
	}

	for (std::thread &thread : threads) {
		thread.join();
	}

	EXPECT_EQ((size_t)(threadCount * voxelsPerThread), v.size());
	EXPECT_TRUE(v.hasVoxel(0, 0, 0));
	EXPECT_TRUE(v.hasVoxel(0, 1, 256));
	EXPECT_TRUE(v.hasVoxel(voxelsPerThread - 1, 3, 256 + voxelsPerThread - 1));
	v.clear();
	EXPECT_EQ(0u, v.size());
}

TEST_F(ConcurrentSparseVolumeTest, testParallelFillReadRemove) {
	ConcurrentSparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 3);
	const int size = 40;
	const int offset = -20;
	app::for_parallel(0, size, [&](int start, int end) {
		for (int z = start; z < end; ++z) {
			for (int y = 0; y < size; ++y) {
				v.setVoxelsRow(offset, offset + y, offset + z, size, voxel);
			}
		}
	});
	ASSERT_EQ((size_t)(size * size * size), v.size());
	const Region region = v.calculateRegion();
	EXPECT_EQ(glm::ivec3(offset), region.getLowerCorner());
	EXPECT_EQ(glm::ivec3(offset + size - 1), region.getUpperCorner());

	core::AtomicInt mismatches{0};
	app::for_parallel(0, size, [&](int start, int end) {
		for (int z = start; z < end; ++z) {
			for (int y = 0; y < size; ++y) {
				for (int x = 0; x < size; ++x) {
					const glm::ivec3 pos(offset + x, offset + y, offset + z);
					if (v.voxel(pos).getColor() != 3) {
						mismatches.increment(1);
					}
					// remove every second voxel while other threads are reading
					if ((x + y + z) % 2 == 0) {
						v.setVoxel(pos, voxel::Voxel());
					}
				}
			}
		}
	});
	EXPECT_EQ(0, (int)mismatches);
	EXPECT_EQ((size_t)(size * size * size / 2), v.size());
	EXPECT_FALSE(v.hasVoxel(offset, offset, offset));
	EXPECT_TRUE(v.hasVoxel(offset + 1, offset, offset));
}

TEST_F(ConcurrentSparseVolumeTest, testCopyToRegion) {
	ConcurrentSparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 1);
	ASSERT_TRUE(v.setVoxel(1, 1, 1, voxel));
	ASSERT_TRUE(v.setVoxel(5, 5, 5, voxel));
	ASSERT_TRUE(v.setVoxel(-300, 1, 1, voxel));
	ASSERT_TRUE(v.setVoxel(600, 1, 1, voxel));
	voxel::RawVolume rv(voxel::Region(0, 7));
	v.copyTo(rv, voxel::Region(0, 3));
	EXPECT_EQ(1, voxelutil::countVoxels(rv));
	EXPECT_TRUE(voxel::isBlocked(rv.voxel(1, 1, 1).getMaterial()));
}

TEST_F(ConcurrentSparseVolumeTest, testSamplerMovesAcrossChunks) {
	ConcurrentSparseVolume v;
	auto colorAt = [](int x, int y, int z) { return (uint8_t)(1 + ((x * 7 + y * 13 + z * 29) & 0x7f)); };
	for (int i = 250; i <= 260; ++i) {
		v.setVoxel(i, 0, 0, voxel::createVoxel(VoxelType::Generic, colorAt(i, 0, 0)));
		v.setVoxel(0, -i, 0, voxel::createVoxel(VoxelType::Generic, colorAt(0, -i, 0)));
		v.setVoxel(0, 0, i, voxel::createVoxel(VoxelType::Generic, colorAt(0, 0, i)));
	}
	ConcurrentSparseVolume::Sampler sampler(v);
	sampler.setPosition(250, 0, 0);
	for (int i = 250; i <= 260; ++i) {
		EXPECT_EQ(colorAt(i, 0, 0), sampler.voxel().getColor()) << "x " << i;
		sampler.movePositive(math::Axis::X);
	}
	sampler.setPosition(0, -250, 0);
	for (int i = 250; i <= 260; ++i) {
		EXPECT_EQ(colorAt(0, -i, 0), sampler.voxel().getColor()) << "y " << i;
		sampler.moveNegativeY();
	}
	sampler.setPosition(0, 0, 260);
	for (int i = 260; i >= 250; i -= 2) {
		EXPECT_EQ(colorAt(0, 0, i), sampler.voxel().getColor()) << "z " << i;
		sampler.moveNegative(math::Axis::Z, 2);
	}
}

TEST_F(ConcurrentSparseVolumeTest, testSamplerLeavesAndReentersRegion) {
	ConcurrentSparseVolume v(voxel::Region(0, 3));
	v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 1));
	v.setVoxel(3, 0, 0, voxel::createVoxel(VoxelType::Generic, 2));
	ConcurrentSparseVolume::Sampler sampler(v);
	EXPECT_TRUE(sampler.setPosition(3, 0, 0));
	sampler.movePositiveX();
	EXPECT_FALSE(sampler.currentPositionValid());
	EXPECT_FALSE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 3)));
	EXPECT_TRUE(voxel::isAir(sampler.voxel().getMaterial()));
	EXPECT_EQ(2, sampler.peekVoxel1nx0py0pz().getColor());
	sampler.moveNegativeX();
	EXPECT_TRUE(sampler.currentPositionValid());
	EXPECT_EQ(2, sampler.voxel().getColor());
	sampler.moveNegativeX(3);
	EXPECT_EQ(1, sampler.voxel().getColor());
	sampler.moveNegativeY();
	EXPECT_FALSE(sampler.currentPositionValid());
	sampler.movePositiveY();
	EXPECT_EQ(1, sampler.voxel().getColor());
}

TEST_F(ConcurrentSparseVolumeTest, testSetVoxelsClipsToRegion) {
	ConcurrentSparseVolume v(voxel::Region(0, 3));
	const voxel::Voxel column[6] = {
		voxel::createVoxel(VoxelType::Generic, 1), voxel::createVoxel(VoxelType::Generic, 2),
		voxel::createVoxel(VoxelType::Generic, 3), voxel::createVoxel(VoxelType::Generic, 4),
		voxel::createVoxel(VoxelType::Generic, 5), voxel::createVoxel(VoxelType::Generic, 6)};
	// starts two voxels below the lower corner on every axis - the first two column entries are below the region
	voxel::setVoxels(v, -2, -2, -2, 6, 6, column, 6);
	EXPECT_EQ(64u, v.size());
	EXPECT_EQ(3, v.voxel(0, 0, 0).getColor());
	EXPECT_EQ(4, v.voxel(0, 1, 0).getColor());
	EXPECT_EQ(6, v.voxel(3, 3, 3).getColor());
}

} // namespace voxel
