/**
 * @file
 */

#include "voxelworld/FilePersister.h"

#include "AbstractVoxelTest.h"

namespace voxelworld {

class WorldPersisterTest: public AbstractVoxelTest {
};

TEST_F(WorldPersisterTest, testSaveLoad) {
	FilePersister persister;
	ASSERT_TRUE(persister.save(_ctx.chunk().get(), _seed)) << "Could not save volume chunk";
	_volData.flushAll();
	ASSERT_TRUE(persister.load(_ctx.chunk().get(), _seed)) << "Could not load volume chunk";
	ASSERT_EQ(voxel::VoxelType::Grass, _volData.voxel(32, 32, 32).getMaterial());
}

}
