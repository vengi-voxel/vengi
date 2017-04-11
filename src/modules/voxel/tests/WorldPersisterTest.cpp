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
	ASSERT_TRUE(persister.save(_ctx.getChunk().get(), _seed)) << "Could not save volume chunk";

	const voxel::Region& region = _ctx.getRegion();
	const std::string& filename = persister.getWorldName(region, _seed);
	const core::App* app = core::App::getInstance();
	const io::FilesystemPtr& filesystem = app->filesystem();
	ASSERT_TRUE(filesystem->open(filename)->exists()) << "Nothing was written into " << filename;

	const PagedVolume::ChunkPtr& chunk2 = _volData.getChunk(glm::ivec3(128, 0, 128));
	ASSERT_TRUE(_ctx.getChunk() != chunk2) << "Chunks should be different";
	const PagedVolumeWrapper ctx(&_volData, chunk2, region);
	_ctx = ctx;
	ASSERT_TRUE(persister.load(_ctx.getChunk().get(), _seed)) << "Could not load volume chunk";
	ASSERT_EQ(VoxelType::Grass, _volData.getVoxel(32, 32, 32).getMaterial());
}

}
