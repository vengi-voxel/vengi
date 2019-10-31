/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxelworld/WorldPersister.h"

namespace voxelworld {

class WorldPersisterTest: public AbstractVoxelTest {
};

TEST_F(WorldPersisterTest, testSaveLoad) {
	WorldPersister persister;
	ASSERT_TRUE(persister.save(_ctx.chunk().get(), _seed)) << "Could not save volume chunk";

	const voxel::Region region = _ctx.region();
	const std::string& filename = persister.getWorldName(region, _seed);
	const core::App* app = _testApp;
	const io::FilesystemPtr& filesystem = app->filesystem();
	ASSERT_TRUE(filesystem->open(filename)->exists()) << "Nothing was written into " << filename;
	_volData.flushAll();
	ASSERT_TRUE(persister.load(_ctx.chunk().get(), _seed)) << "Could not load volume chunk";
	ASSERT_EQ(voxel::VoxelType::Grass, _volData.voxel(32, 32, 32).getMaterial());
}

}
