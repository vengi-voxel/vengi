#include "AbstractVoxelTest.h"
#include "voxel/World.h"
#include <chrono>

namespace voxel {

class WorldTest: public AbstractVoxelTest {
};

TEST_F(WorldTest, DISABLED_testExtraction) {
	World world;
	int expected = 0;
	for (int i = 0; i < 1024; ++i) {
		const glm::ivec3 pos { i, 0, i };
		if (world.scheduleMeshExtraction(pos))
			++expected;
	}

	ASSERT_GT(expected, 10);

	int extracted = 0;
	auto start = std::chrono::high_resolution_clock::now();
	for (;;) {
		DecodedMeshData meshData;
		while (!world.pop(meshData)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed = end - start;
			const double millis = elapsed.count();
			ASSERT_LT(millis, 120 * 1000);
		}
		++extracted;
		if (extracted == expected) {
			break;
		}
	}
}

TEST_F(WorldTest, testRegion) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(15, 15, 15);
	Region region(mins, maxs);
	ASSERT_TRUE(region.containsPoint(mins));
	ASSERT_TRUE(region.containsPoint(maxs));
	ASSERT_FALSE(region.containsPoint(mins, 1));
	ASSERT_FALSE(region.containsPoint(maxs, 1));
	ASSERT_FALSE(region.containsPoint(maxs + 1));
	ASSERT_TRUE(region.containsRegion(region));
	ASSERT_FALSE(region.containsRegion(region, 1));
}

TEST_F(WorldTest, testChunkAndGridPos) {
	World world;
	const int chunkSize = world.getChunkSize();
	const int halfChunkSize = chunkSize / 2;
	// 0 - 63 => chunk 0
	// -64 - -1 => chunk -1

	// 0 - 63 => gridpos 0
	// -64 - -1 => gridpos -64

	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(chunkSize, chunkSize, chunkSize));
		ASSERT_EQ(glm::ivec3(1, 1, 1), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(chunkSize, chunkSize, chunkSize));
		ASSERT_EQ(glm::ivec3(chunkSize, 0, chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(chunkSize + 1, chunkSize + 1, chunkSize + 1));
		ASSERT_EQ(glm::ivec3(1, 1, 1), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(chunkSize + 1, chunkSize + 1, chunkSize + 1));
		ASSERT_EQ(glm::ivec3(chunkSize, 0, chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(chunkSize - 1, chunkSize - 1, chunkSize - 1));
		ASSERT_EQ(glm::ivec3(0, 0, 0), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(chunkSize - 1, chunkSize - 1, chunkSize - 1));
		ASSERT_EQ(glm::ivec3(0, 0, 0), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(-chunkSize, -chunkSize, -chunkSize));
		ASSERT_EQ(glm::ivec3(-1, -1, -1), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(-chunkSize, -chunkSize, -chunkSize));
		ASSERT_EQ(glm::ivec3(-chunkSize, -0, -chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(-(chunkSize + 1), -(chunkSize + 1), -(chunkSize + 1)));
		ASSERT_EQ(glm::ivec3(-2, -2, -2), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(-(chunkSize + 1), -(chunkSize + 1), -(chunkSize + 1)));
		ASSERT_EQ(glm::ivec3(-2 * chunkSize, -0, -2 * chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(-halfChunkSize, halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec3(-1, 0, 0), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(-halfChunkSize, halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec3(-chunkSize, 0, 0), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(-halfChunkSize, -halfChunkSize, -halfChunkSize));
		ASSERT_EQ(glm::ivec3(-1, -1, -1), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(-halfChunkSize, -halfChunkSize, -halfChunkSize));
		ASSERT_EQ(glm::ivec3(-chunkSize, 0, -chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(halfChunkSize, halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec3(0, 0, 0), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(halfChunkSize, halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec3(0, 0, 0), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(2 * chunkSize + halfChunkSize, 2 * chunkSize + halfChunkSize, 2 * chunkSize + halfChunkSize));
		ASSERT_EQ(glm::ivec3(2, 2, 2), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(2 * chunkSize + halfChunkSize, 2 * chunkSize + halfChunkSize, 2 * chunkSize + halfChunkSize));
		ASSERT_EQ(glm::ivec3(2 * chunkSize, 0, 2 * chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(halfChunkSize, 0, halfChunkSize));
		ASSERT_EQ(glm::ivec3(0, 0, 0), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(halfChunkSize, 0, halfChunkSize));
		ASSERT_EQ(glm::ivec3(0, 0, 0), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(halfChunkSize, MAX_HEIGHT - 1, halfChunkSize));
		ASSERT_EQ(glm::ivec3(0, MAX_HEIGHT / chunkSize, 0), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(halfChunkSize, MAX_HEIGHT - 1, halfChunkSize));
		ASSERT_EQ(glm::ivec3(0, 0, 0), gridPos);
	}
}

}
