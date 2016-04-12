#include "core/tests/AbstractTest.h"
#include "voxel/World.h"

namespace voxel {

class WorldTest: public core::AbstractTest {
};

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
		ASSERT_EQ(glm::ivec3(chunkSize, chunkSize, chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(chunkSize + 1, chunkSize + 1, chunkSize + 1));
		ASSERT_EQ(glm::ivec3(1, 1, 1), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(chunkSize + 1, chunkSize + 1, chunkSize + 1));
		ASSERT_EQ(glm::ivec3(chunkSize, chunkSize, chunkSize), gridPos);
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
		ASSERT_EQ(glm::ivec3(-chunkSize, -chunkSize, -chunkSize), gridPos);
	}
	{
		const glm::ivec3& chunkPos = world.getChunkPos(glm::ivec3(-(chunkSize + 1), -(chunkSize + 1), -(chunkSize + 1)));
		ASSERT_EQ(glm::ivec3(-2, -2, -2), chunkPos);
		const glm::ivec3& gridPos = world.getGridPos(glm::ivec3(-(chunkSize + 1), -(chunkSize + 1), -(chunkSize + 1)));
		ASSERT_EQ(glm::ivec3(-2 * chunkSize, -2 * chunkSize, -2 * chunkSize), gridPos);
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
		ASSERT_EQ(glm::ivec3(-chunkSize, -chunkSize, -chunkSize), gridPos);
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
		ASSERT_EQ(glm::ivec3(2 * chunkSize, 2 * chunkSize, 2 * chunkSize), gridPos);
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
		ASSERT_EQ(glm::ivec3(0, chunkPos.y * chunkSize, 0), gridPos);
	}
}

}
