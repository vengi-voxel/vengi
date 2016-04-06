#include "core/tests/AbstractTest.h"
#include "voxel/World.h"

namespace voxel {

class WorldTest: public core::AbstractTest {
};

TEST_F(WorldTest, testChunkAndGridPos) {
	World world;
	const int chunkSize = world.getChunkSize();
	const int halfChunkSize = chunkSize / 2;
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(-halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(-1, 0), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(-halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(-chunkSize, 0), gridPos);
	}
	{
		const glm::ivec2& chunkPos = world.getChunkPos(glm::ivec2(halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(1, 0), chunkPos);
		const glm::ivec2& gridPos = world.getGridPos(glm::ivec2(halfChunkSize, halfChunkSize));
		ASSERT_EQ(glm::ivec2(0, 0), gridPos);
	}
}

}
