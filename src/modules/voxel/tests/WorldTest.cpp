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
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(chunkSize, chunkSize));
		ASSERT_EQ(glm::ivec2(1, 1), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(chunkSize, chunkSize));
		ASSERT_EQ(glm::ivec2(chunkSize, chunkSize), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(chunkSize + 1, chunkSize + 1));
		ASSERT_EQ(glm::ivec2(1, 1), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(chunkSize + 1, chunkSize + 1));
		ASSERT_EQ(glm::ivec2(chunkSize, chunkSize), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(chunkSize - 1, chunkSize - 1));
		ASSERT_EQ(glm::ivec2(0, 0), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(chunkSize - 1, chunkSize - 1));
		ASSERT_EQ(glm::ivec2(0, 0), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(-chunkSize, -chunkSize));
		ASSERT_EQ(glm::ivec2(-1, -1), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(-chunkSize, -chunkSize));
		ASSERT_EQ(glm::ivec2(-chunkSize, -chunkSize), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(-(chunkSize + 1), -(chunkSize + 1)));
		ASSERT_EQ(glm::ivec2(-2, -2), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(-(chunkSize + 1), -(chunkSize + 1)));
		ASSERT_EQ(glm::ivec2(-2 * chunkSize, -2 * chunkSize), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(-halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(-1, 0), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(-halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(-chunkSize, 0), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(-halfChunkSize, -halfChunkSize));
		ASSERT_EQ(glm::ivec2(-1, -1), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(-halfChunkSize, -halfChunkSize));
		ASSERT_EQ(glm::ivec2(-chunkSize, -chunkSize), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(0, 0), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(0, 0), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(2 * chunkSize + halfChunkSize, 2 * chunkSize + halfChunkSize));
		ASSERT_EQ(glm::ivec2(2, 2), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(2 * chunkSize + halfChunkSize, 2 * chunkSize + halfChunkSize));
		ASSERT_EQ(glm::ivec2(2 * chunkSize, 2 * chunkSize), gridPos);
	}
}

}
