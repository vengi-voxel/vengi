#include "AbstractVoxelTest.h"
#include "voxel/WorldPersister.h"

namespace voxel {

class WorldPersisterTest: public AbstractVoxelTest {
};

TEST_F(WorldPersisterTest, testSaveLoad) {
	const voxel::Region region(glm::ivec3(0, 0, 0), glm::ivec3(63, 63, 63));
	const long seed = 0;
	Pager pager;
	PagedVolume volData(&pager, 256 * 1024 * 1024, 64);
	WorldPersister persister;
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = volData.getChunk(region.getLowerCorner());
	ASSERT_TRUE(ctx.chunk != nullptr) << "Could not get chunk";
	ASSERT_TRUE(persister.save(ctx, seed)) << "Could not save volume chunk";

	const std::string& filename = persister.getWorldName(region, seed);
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	ASSERT_TRUE(filesystem->open(filename)->exists()) << "Nothing was written into " << filename;

	const PagedVolume::Chunk* chunk = ctx.chunk;
	ctx.chunk = volData.getChunk(glm::ivec3(128, 0, 128));
	ASSERT_TRUE(chunk != ctx.chunk) << "Chunks should be different";
	ASSERT_TRUE(persister.load(ctx, seed)) << "Could not load volume chunk";
	ASSERT_EQ(Grass, ctx.chunk->getVoxel(32, 32, 32).getMaterial());
}

}
