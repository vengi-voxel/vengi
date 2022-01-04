/**
 * @file
 */

#include "voxelworld/FilePersister.h"
#include "AbstractVoxelWorldTest.h"

namespace voxelworld {

class FilePersisterTest: public AbstractVoxelWorldTest {
};

TEST_F(FilePersisterTest, testSaveLoad) {
	FilePersister persister;
	ASSERT_TRUE(persister.save(_ctx.chunk(), _seed)) << "Could not save volume chunk";
	_volData.flushAll();
	ASSERT_TRUE(persister.load(_ctx.chunk(), _seed)) << "Could not load volume chunk";
	ASSERT_EQ(voxel::VoxelType::Grass, _volData.voxel(32, 32, 32).getMaterial());
}

}
