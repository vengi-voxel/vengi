/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/WorldPersister.h"

namespace voxel {

class WorldPersisterTest: public AbstractVoxelTest {
};

TEST_F(WorldPersisterTest, testSaveLoad) {
	WorldPersister persister;
	ASSERT_TRUE(persister.save(_ctx, _seed)) << "Could not save volume chunk";

	const std::string& filename = persister.getWorldName(_ctx.region, _seed);
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	ASSERT_TRUE(filesystem->open(filename)->exists()) << "Nothing was written into " << filename;

	PagedVolume::Chunk* chunk2 = _volData.getChunk(glm::ivec3(128, 0, 128));
	ASSERT_TRUE(_ctx.getChunk() != chunk2) << "Chunks should be different";
	const GeneratorContext ctx(_ctx.getVolume(), chunk2, _ctx.region);
	_ctx = ctx;
	ASSERT_TRUE(persister.load(_ctx, _seed)) << "Could not load volume chunk";
	ASSERT_EQ(VoxelType::Grass1, _volData.getVoxel(32, 32, 32).getMaterial());
}

}
