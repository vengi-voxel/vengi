#include "AbstractVoxelTest.h"
#include "voxel/WorldPersister.h"

namespace voxel {

class WorldPersisterTest: public AbstractVoxelTest {
};

TEST_F(WorldPersisterTest, testSaveLoad) {
	const voxel::Region region(glm::ivec3(0, 0, 0), glm::ivec3(63, 63, 63));
	Pager pager;
	PagedVolume volData(&pager, 256 * 1024 * 1024, 64);
	WorldPersister persister;
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = volData.getChunk(region.getLowerCorner());
	ASSERT_TRUE(ctx.chunk != nullptr) << "Could not get chunk";
	ASSERT_TRUE(persister.save(ctx, 0)) << "Could not save volume chunk";
	const PagedVolume::Chunk* chunk = ctx.chunk;
	ctx.chunk = volData.getChunk(glm::ivec3(128, 0, 128));
	ASSERT_TRUE(chunk != ctx.chunk) << "Chunks should be different";
	ASSERT_TRUE(persister.load(ctx, 0)) << "Could not load volume chunk";
	ASSERT_EQ(Grass, ctx.chunk->getVoxel(32, 32, 32).getMaterial());
}

}
