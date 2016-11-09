/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/polyvox/VolumeRotator.h"

namespace voxel {

class VolumeRotatorTest: public AbstractVoxelTest {
};

TEST_F(VolumeRotatorTest, testRotate90Y) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCentre();
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock1)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock2)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock3)));

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 90, 0));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region& rotatedRegion = rotated->getEnclosingRegion();
	ASSERT_EQ(rotatedRegion, region) << "Rotating by 90 degree should not increase the size of the volume "
			<< str(rotatedRegion) << " " << str(region);
	glm::ivec3 rotPos = rotatedRegion.getCentre();
	ASSERT_EQ(voxel::VoxelType::Rock1, rotated->getVoxel(rotPos.x, rotPos.y++, rotPos.z).getMaterial())
			<< str(rotatedRegion) << " " << str(region);
	ASSERT_EQ(voxel::VoxelType::Rock2, rotated->getVoxel(rotPos.x, rotPos.y++, rotPos.z).getMaterial())
			<< str(rotatedRegion) << " " << str(region);
	ASSERT_EQ(voxel::VoxelType::Rock3, rotated->getVoxel(rotPos.x, rotPos.y++, rotPos.z).getMaterial())
			<< str(rotatedRegion) << " " << str(region);
}

TEST_F(VolumeRotatorTest, testRotate45Y) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCentre();
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock1)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock2)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock3)));

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 45, 0));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region& rotatedRegion = rotated->getEnclosingRegion();
	ASSERT_NE(rotatedRegion, region) << "Rotating by 45 degree should increase the size of the volume "
			<< str(rotatedRegion) << " " << str(region);
}

TEST_F(VolumeRotatorTest, testRotate45YNoExtend) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCentre();
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock1)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock2)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock3)));

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 45, 0), false);
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region& rotatedRegion = rotated->getEnclosingRegion();
	ASSERT_EQ(rotatedRegion, region) << "This rotation was forced to not exceed the source bounds "
			<< str(rotatedRegion) << " " << str(region);
}

}
