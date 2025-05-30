/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxelutil/VolumeMover.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxelutil {

class VolumeMoverTest: public app::AbstractTest {
};


TEST_F(VolumeMoverTest, testMove) {
	voxel::Region regionBig = voxel::Region(0, 5);
	voxel::RawVolume bigVolume(regionBig);
	ASSERT_TRUE(bigVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(bigVolume.setVoxel(1, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));

	voxel::RawVolume newVolume(bigVolume.region());
	voxel::RawVolumeMoveWrapper wrapper(&newVolume);
	glm::ivec3 offsets(1, 0, 0);
	voxelutil::moveVolume(&wrapper, &bigVolume, offsets);

	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(1, 0, 0).getMaterial())) << "Expected to find a voxel at (1, 0, 0)\n";
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(2, 1, 0).getMaterial())) << "Expected to find a voxel at (2, 1, 0)\n";
}

}
