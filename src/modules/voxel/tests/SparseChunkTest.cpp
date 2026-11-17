/**
 * @file
 */

#include "voxel/SparseChunk.h"
#include "app/tests/AbstractTest.h"
#include "voxel/Voxel.h"

namespace voxel {

class SparseChunkTest : public app::AbstractTest {};

TEST_F(SparseChunkTest, testSetAndRemove) {
	SparseChunk chunk;
	const Voxel voxel = createVoxel(VoxelType::Generic, 3);
	EXPECT_EQ(nullptr, chunk.voxel(5, 6, 7));
	EXPECT_FALSE(chunk.hasVoxel(5, 6, 7));

	EXPECT_TRUE(chunk.setVoxel(5, 6, 7, voxel));
	EXPECT_FALSE(chunk.setVoxel(5, 6, 7, voxel));
	EXPECT_EQ(1, chunk.size());
	EXPECT_TRUE(chunk.hasVoxel(5, 6, 7));
	ASSERT_NE(nullptr, chunk.voxel(5, 6, 7));
	EXPECT_EQ(3, chunk.voxel(5, 6, 7)->getColor());

	// same brick, not stored - slot holds air
	ASSERT_NE(nullptr, chunk.voxel(4, 6, 7));
	EXPECT_TRUE(isAir(chunk.voxel(4, 6, 7)->getMaterial()));
	EXPECT_FALSE(chunk.hasVoxel(4, 6, 7));

	EXPECT_FALSE(chunk.removeVoxel(4, 6, 7));
	EXPECT_TRUE(chunk.removeVoxel(5, 6, 7));
	EXPECT_FALSE(chunk.removeVoxel(5, 6, 7));
	EXPECT_TRUE(chunk.empty());
	EXPECT_EQ(nullptr, chunk.brick(5, 6, 7)) << "Empty brick should get released";
}

TEST_F(SparseChunkTest, testRemoveKeepsMemory) {
	SparseChunk chunk(true);
	const Voxel voxel = createVoxel(VoxelType::Generic, 3);
	EXPECT_TRUE(chunk.setVoxel(1, 1, 1, voxel));
	const Voxel *slot = chunk.voxel(1, 1, 1);
	EXPECT_TRUE(chunk.removeVoxel(1, 1, 1, false));
	EXPECT_EQ(slot, chunk.voxel(1, 1, 1));
	EXPECT_TRUE(isAir(slot->getMaterial()));
	EXPECT_TRUE(chunk.empty());
}

TEST_F(SparseChunkTest, testRemoveResetsNormal) {
	SparseChunk chunk;
	EXPECT_TRUE(chunk.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 1, 5)));
	EXPECT_TRUE(chunk.setVoxel(2, 1, 1, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(chunk.removeVoxel(1, 1, 1));
	EXPECT_TRUE(chunk.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 2)));
	EXPECT_EQ(NO_NORMAL, chunk.voxel(1, 1, 1)->getNormal()) << "A removed voxel must not leak its normal";
}

TEST_F(SparseChunkTest, testOverwriteKeepsVoxelAssignmentSemantics) {
	SparseChunk chunk;
	EXPECT_TRUE(chunk.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 1, 5)));
	EXPECT_FALSE(chunk.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 2)));
	Voxel expected = createVoxel(VoxelType::Generic, 1, 5);
	expected = createVoxel(VoxelType::Generic, 2);
	EXPECT_EQ(expected.getNormal(), chunk.voxel(1, 1, 1)->getNormal());
	EXPECT_EQ(2, chunk.voxel(1, 1, 1)->getColor());
}

TEST_F(SparseChunkTest, testBoundaries) {
	SparseChunk chunk;
	const Voxel voxel = createVoxel(VoxelType::Generic, 1);
	const int coords[] = {0, 7, 8, 31, 32, 63, 64, 255};
	int expected = 0;
	for (int x : coords) {
		for (int y : coords) {
			for (int z : coords) {
				EXPECT_TRUE(chunk.setVoxel(x, y, z, voxel));
				++expected;
			}
		}
	}
	EXPECT_EQ(expected, chunk.size());
	for (int x : coords) {
		for (int y : coords) {
			for (int z : coords) {
				EXPECT_TRUE(chunk.hasVoxel(x, y, z)) << x << ":" << y << ":" << z;
			}
		}
	}
	EXPECT_FALSE(chunk.hasVoxel(1, 0, 0));
	EXPECT_FALSE(chunk.hasVoxel(254, 255, 255));

	glm::ivec3 mins, maxs;
	ASSERT_TRUE(chunk.calculateBounds(mins, maxs));
	EXPECT_EQ(glm::ivec3(0), mins);
	EXPECT_EQ(glm::ivec3(255), maxs);

	int visited = 0;
	chunk.visit([&](int x, int y, int z, const Voxel &v) {
		EXPECT_TRUE(chunk.hasVoxel(x, y, z)) << x << ":" << y << ":" << z;
		EXPECT_EQ(1, v.getColor());
		++visited;
	});
	EXPECT_EQ(expected, visited);

	for (int x : coords) {
		for (int y : coords) {
			for (int z : coords) {
				EXPECT_TRUE(chunk.removeVoxel(x, y, z));
			}
		}
	}
	EXPECT_TRUE(chunk.empty());
	EXPECT_FALSE(chunk.calculateBounds(mins, maxs));
}

TEST_F(SparseChunkTest, testCalculateBoundsInsideBrick) {
	SparseChunk chunk;
	const Voxel voxel = createVoxel(VoxelType::Generic, 1);
	chunk.setVoxel(10, 20, 30, voxel);
	chunk.setVoxel(13, 17, 29, voxel);
	glm::ivec3 mins, maxs;
	ASSERT_TRUE(chunk.calculateBounds(mins, maxs));
	EXPECT_EQ(glm::ivec3(10, 17, 29), mins);
	EXPECT_EQ(glm::ivec3(13, 20, 30), maxs);
}

TEST_F(SparseChunkTest, testSetVoxelsRow) {
	SparseChunk chunk;
	const Voxel voxel = createVoxel(VoxelType::Generic, 4);
	EXPECT_EQ(1, chunk.setVoxelsRow(5, 2, 3, 1, voxel));
	EXPECT_EQ(199, chunk.setVoxelsRow(3, 2, 3, 200, voxel));
	EXPECT_EQ(200, chunk.size());
	for (int x = 0; x < SparseChunk::Side; ++x) {
		EXPECT_EQ(x >= 3 && x < 203, chunk.hasVoxel(x, 2, 3)) << x;
	}
	EXPECT_EQ(0, chunk.setVoxelsRow(3, 2, 3, 200, voxel));
	EXPECT_EQ(53, chunk.setVoxelsRow(203, 2, 3, 53, voxel));
	EXPECT_EQ(253, chunk.size());
}

TEST_F(SparseChunkTest, testClear) {
	SparseChunk chunk;
	chunk.setVoxelsRow(0, 0, 0, 256, createVoxel(VoxelType::Generic, 1));
	chunk.clear();
	EXPECT_TRUE(chunk.empty());
	EXPECT_EQ(nullptr, chunk.brick(0, 0, 0));
	EXPECT_TRUE(chunk.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_EQ(1, chunk.size());
}

static uint8_t colorForIndex(int idx) {
	return (uint8_t)(1 + (idx * 37) % 250);
}

// visits all 512 positions of the first brick in a scrambled order
static int scrambledIndex(int i) {
	return (i * 167) & (SparseChunk::BrickVoxels - 1);
}

static void expectBrickContent(const SparseChunk &chunk, const bool *present) {
	for (int idx = 0; idx < SparseChunk::BrickVoxels; ++idx) {
		const int x = idx & 7;
		const int y = (idx >> 3) & 7;
		const int z = idx >> 6;
		ASSERT_EQ(present[idx], chunk.hasVoxel(x, y, z)) << idx;
		const Voxel *v = chunk.voxel(x, y, z);
		ASSERT_NE(nullptr, v);
		if (present[idx]) {
			ASSERT_EQ(colorForIndex(idx), v->getColor()) << idx;
		} else {
			ASSERT_TRUE(isAir(v->getMaterial())) << idx;
		}
	}
}

TEST_F(SparseChunkTest, testPackedInsertRemoveAndPromotion) {
	SparseChunk chunk;
	bool present[SparseChunk::BrickVoxels] = {};
	// stays packed, then grows over all packed capacities and gets dense
	for (int i = 0; i < SparseChunk::PackedMaxVoxels + 10; ++i) {
		const int idx = scrambledIndex(i);
		ASSERT_TRUE(chunk.setVoxel(idx & 7, (idx >> 3) & 7, idx >> 6, createVoxel(VoxelType::Generic, colorForIndex(idx))));
		present[idx] = true;
		if (i == SparseChunk::InlineVoxels || i == SparseChunk::PackedMaxVoxels - 1 || i == SparseChunk::PackedMaxVoxels) {
			expectBrickContent(chunk, present);
			ASSERT_EQ(i >= SparseChunk::PackedMaxVoxels, chunk.brick(0, 0, 0)->dense()) << i;
		}
	}
	EXPECT_EQ(SparseChunk::PackedMaxVoxels + 10, chunk.size());
	expectBrickContent(chunk, present);
}

TEST_F(SparseChunkTest, testPackedRemove) {
	SparseChunk chunk;
	bool present[SparseChunk::BrickVoxels] = {};
	for (int i = 0; i < 40; ++i) {
		const int idx = scrambledIndex(i);
		chunk.setVoxel(idx & 7, (idx >> 3) & 7, idx >> 6, createVoxel(VoxelType::Generic, colorForIndex(idx)));
		present[idx] = true;
	}
	for (int i = 0; i < 40; i += 3) {
		const int idx = scrambledIndex(i);
		ASSERT_TRUE(chunk.removeVoxel(idx & 7, (idx >> 3) & 7, idx >> 6));
		present[idx] = false;
	}
	ASSERT_FALSE(chunk.brick(0, 0, 0)->dense());
	expectBrickContent(chunk, present);
	// overwrite keeps the packed position
	const int idx = scrambledIndex(1);
	EXPECT_FALSE(chunk.setVoxel(idx & 7, (idx >> 3) & 7, idx >> 6, createVoxel(VoxelType::Generic, colorForIndex(idx))));
	expectBrickContent(chunk, present);
}

TEST_F(SparseChunkTest, testPackedVisitOrder) {
	SparseChunk chunk;
	for (int i = 0; i < 20; ++i) {
		const int idx = scrambledIndex(i);
		chunk.setVoxel(idx & 7, (idx >> 3) & 7, idx >> 6, createVoxel(VoxelType::Generic, colorForIndex(idx)));
	}
	int visited = 0;
	int lastIdx = -1;
	chunk.visit([&](int x, int y, int z, const Voxel &v) {
		const int idx = SparseChunk::voxelIndex(x, y, z);
		EXPECT_GT(idx, lastIdx);
		EXPECT_EQ(colorForIndex(idx), v.getColor()) << idx;
		lastIdx = idx;
		++visited;
	});
	EXPECT_EQ(20, visited);
}

TEST_F(SparseChunkTest, testSetVoxelsRowPackedToDense) {
	SparseChunk chunk;
	const Voxel voxel = createVoxel(VoxelType::Generic, 2);
	// 8 rows of 8 voxels fill the packed capacity, the 9th row promotes the brick
	for (int y = 0; y < 8; ++y) {
		EXPECT_EQ(8, chunk.setVoxelsRow(0, y, 0, 8, voxel));
	}
	EXPECT_FALSE(chunk.brick(0, 0, 0)->dense());
	EXPECT_EQ(8, chunk.setVoxelsRow(0, 0, 1, 8, voxel));
	EXPECT_TRUE(chunk.brick(0, 0, 0)->dense());
	EXPECT_EQ(0, chunk.setVoxelsRow(0, 0, 1, 8, voxel));
	EXPECT_EQ(72, chunk.size());
	int visited = 0;
	chunk.visit([&](int, int, int, const Voxel &v) {
		EXPECT_EQ(2, v.getColor());
		++visited;
	});
	EXPECT_EQ(72, visited);
}

TEST_F(SparseChunkTest, testBrickReuse) {
	SparseChunk chunk;
	for (int i = 0; i < SparseChunk::PoolBlockBricks + 2; ++i) {
		ASSERT_TRUE(chunk.setVoxel(i * SparseChunk::BrickSide % SparseChunk::Side, i / 32 * 8, 0,
								   createVoxel(VoxelType::Generic, 1)));
	}
	// release a brick with a heap array and one with inline storage
	for (int x = 0; x < 8; ++x) {
		chunk.setVoxel(x, 0, 1, createVoxel(VoxelType::Generic, 1));
	}
	for (int x = 0; x < 8; ++x) {
		ASSERT_TRUE(chunk.removeVoxel(x, 0, 1));
	}
	ASSERT_TRUE(chunk.removeVoxel(0, 0, 0));
	ASSERT_EQ(nullptr, chunk.brick(0, 0, 0));
	ASSERT_TRUE(chunk.removeVoxel(8, 0, 0));
	ASSERT_EQ(nullptr, chunk.brick(8, 0, 0));
	const int expected = SparseChunk::PoolBlockBricks;
	EXPECT_EQ(expected, chunk.size());

	// reuses the released slots - they must start empty
	EXPECT_TRUE(chunk.setVoxel(200, 200, 200, createVoxel(VoxelType::Generic, 7)));
	EXPECT_TRUE(chunk.setVoxel(100, 100, 100, createVoxel(VoxelType::Generic, 8)));
	EXPECT_EQ(1, chunk.brick(200, 200, 200)->count);
	EXPECT_EQ(7, chunk.voxel(200, 200, 200)->getColor());
	EXPECT_EQ(8, chunk.voxel(100, 100, 100)->getColor());
	EXPECT_FALSE(chunk.hasVoxel(0, 0, 0));
	EXPECT_FALSE(chunk.hasVoxel(201, 200, 200));

	int visited = 0;
	chunk.visit([&](int, int, int, const Voxel &) { ++visited; });
	EXPECT_EQ(expected + 2, visited);
	glm::ivec3 mins, maxs;
	ASSERT_TRUE(chunk.calculateBounds(mins, maxs));
	EXPECT_EQ(200, maxs.y);
	EXPECT_EQ(200, maxs.z);
}

TEST_F(SparseChunkTest, testDenseBricks) {
	SparseChunk chunk(true);
	EXPECT_TRUE(chunk.setVoxel(1, 2, 3, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(chunk.brick(1, 2, 3)->dense());
	const Voxel *slot = chunk.voxel(1, 2, 3);
	for (int x = 0; x < 8; ++x) {
		chunk.setVoxel(x, 0, 0, createVoxel(VoxelType::Generic, 2));
	}
	EXPECT_EQ(slot, chunk.voxel(1, 2, 3));
	EXPECT_EQ(1, slot->getColor());
}

TEST_F(SparseChunkTest, testReadRegionAcrossPackedDenseAndUnallocatedBricks) {
	SparseChunk chunk;
	const Voxel a = createVoxel(VoxelType::Generic, 1, 3);
	const Voxel b = createVoxel(VoxelType::Generic, 9, 6, FlagOutline);
	chunk.setVoxel(0, 6, 6, a); // outside the copied region, before the packed row start
	chunk.setVoxel(6, 6, 6, b);
	chunk.setVoxel(7, 6, 6, createVoxel(VoxelType::Air, 0, 0, FlagOutline));
	for (int z = 0; z < SparseChunk::BrickSide; ++z) {
		for (int y = 0; y < SparseChunk::BrickSide; ++y) {
			chunk.setVoxelsRow(8, y, z, SparseChunk::BrickSide, a);
		}
	}
	ASSERT_TRUE(chunk.brick(8, 0, 0)->dense());
	chunk.setVoxel(9, 7, 7, b);
	chunk.removeVoxel(8, 7, 7);

	const glm::ivec3 mins(5, 6, 6);
	const glm::ivec3 size(14, 4, 4);
	Voxel output[14 * 4 * 4];
	for (Voxel &v : output) {
		v.overwrite(b);
	}
	chunk.readRegion(mins, size, output);
	for (int z = 0; z < size.z; ++z) {
		for (int y = 0; y < size.y; ++y) {
			for (int x = 0; x < size.x; ++x) {
				const Voxel *v = chunk.voxel(mins.x + x, mins.y + y, mins.z + z);
				EXPECT_TRUE(output[x + size.x * (y + size.y * z)] == (v ? *v : Voxel())) << x << ":" << y << ":" << z;
			}
		}
	}
}

TEST_F(SparseChunkTest, testDensePayloadSlotReusedForPackedBrick) {
	SparseChunk chunk;
	const Voxel voxel = createVoxel(VoxelType::Generic, 3);
	for (int y = 0; y < SparseChunk::BrickSide; ++y) {
		chunk.setVoxelsRow(0, y, 0, SparseChunk::BrickSide, voxel);
	}
	ASSERT_TRUE(chunk.setVoxel(0, 0, 1, voxel));
	ASSERT_TRUE(chunk.brick(0, 0, 0)->dense());
	ASSERT_EQ(3, chunk.voxel(0, 0, 1)->getColor());
	chunk.removeVoxel(0, 0, 1);
	for (int y = 0; y < SparseChunk::BrickSide; ++y) {
		for (int x = 0; x < SparseChunk::BrickSide; ++x) {
			chunk.removeVoxel(x, y, 0);
		}
	}
	EXPECT_EQ(nullptr, chunk.voxel(0, 0, 0));
	chunk.setVoxel(16, 5, 2, createVoxel(VoxelType::Generic, 9));
	ASSERT_FALSE(chunk.brick(16, 5, 2)->dense());
	EXPECT_EQ(9, chunk.voxel(16, 5, 2)->getColor());
	EXPECT_TRUE(isAir(chunk.voxel(17, 5, 2)->getMaterial()));
}

TEST_F(SparseChunkTest, testReadBrickRow) {
	SparseChunk chunk;
	const Voxel filled = createVoxel(VoxelType::Generic, 3, 4);
	const Voxel markedAir = createVoxel(VoxelType::Air, 0, 0, FlagOutline);
	Voxel scratch[SparseChunk::BrickSide];
	EXPECT_TRUE(chunk.readBrickRow(0, 1, 1, scratch)[0] == Voxel());
	chunk.setVoxelsRow(8, 1, 1, SparseChunk::BrickSide, filled);
	chunk.setVoxel(0, 1, 1, markedAir);
	chunk.setVoxel(7, 1, 1, filled);
	for (int z = 0; z < 2; ++z) {
		for (int y = 0; y < SparseChunk::BrickSide; ++y) {
			chunk.setVoxelsRow(16, y, z, SparseChunk::BrickSide, filled);
		}
	}
	chunk.removeVoxel(17, 1, 1);

	for (int x = 0; x < 32; x += SparseChunk::BrickSide) {
		const Voxel *row = chunk.readBrickRow(x, 1, 1, scratch);
		for (int i = 0; i < SparseChunk::BrickSide; ++i) {
			const Voxel *point = chunk.voxel(x + i, 1, 1);
			EXPECT_TRUE(row[i] == (point ? *point : Voxel())) << x + i;
		}
	}

	const Voxel edited = createVoxel(VoxelType::Generic, 11, 7);
	chunk.setVoxel(11, 1, 1, edited);
	EXPECT_TRUE(chunk.readBrickRow(8, 1, 1, scratch)[3] == edited);
	for (int y = 0; y < SparseChunk::BrickSide; ++y) {
		chunk.setVoxelsRow(8, y, 0, SparseChunk::BrickSide, filled);
	}
	ASSERT_TRUE(chunk.brick(8, 1, 1)->dense());
	EXPECT_TRUE(chunk.readBrickRow(8, 1, 1, scratch)[3] == edited);
	chunk.removeVoxel(11, 1, 1);
	EXPECT_TRUE(chunk.readBrickRow(8, 1, 1, scratch)[3] == Voxel());
}

} // namespace voxel
