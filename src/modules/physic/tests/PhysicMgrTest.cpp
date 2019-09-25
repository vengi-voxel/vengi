/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "physic/PhysicMgr.h"
#include "voxel/polyvox/RawVolume.h"

namespace physic {

class PhysicMgrTest: public core::AbstractTest {
};

TEST_F(PhysicMgrTest, testInit) {
	PhysicMgr mgr;
	ASSERT_TRUE(mgr.init()) << "Failed to initialize physics";
	mgr.shutdown();
}

TEST_F(PhysicMgrTest, testWorld) {
	PhysicMgr mgr;
	ASSERT_TRUE(mgr.init()) << "Failed to initialize physics";
	voxel::Region region(0, 10);
	voxel::RawVolume volume(region);
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Grass, 1));
	volume.setVoxel(5, 5, 5, voxel::createVoxel(voxel::VoxelType::Grass, 1));
	EXPECT_EQ(2, mgr.addVoxelTree(&volume));
	mgr.shutdown();
}

}
