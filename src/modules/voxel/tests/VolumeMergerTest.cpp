/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/polyvox/VolumeMerger.h"

namespace voxel {

class VolumeMergerTest: public AbstractVoxelTest {
};

TEST_F(VolumeMergerTest, testMergeDifferentSize) {
	voxel::RawVolume smallVolume(voxel::Region(0, 1));
	smallVolume.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass1));

	voxel::RawVolume bigVolume(voxel::Region(0, 10));
	ASSERT_EQ(1, voxel::mergeRawVolumes(&bigVolume, &smallVolume, glm::ivec3(5, 5, 5)))
		<< "The single voxel from the small volume should have been merged into the big volume";
}

}
