/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "AbstractVoxelTest.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Voxel.h"
#include "math/Axis.h"

namespace voxel {

class VolumeRotatorTest: public core::AbstractTest {
protected:
	inline std::string str(const voxel::Region& region) const {
		return "mins(" + glm::to_string(region.getLowerCorner()) + "), maxs(" + glm::to_string(region.getUpperCorner()) + ")";
	}
};

TEST_F(VolumeRotatorTest, testRotateAxisY) {
	const voxel::Region region(-1, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Rock, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, createVoxel(voxel::VoxelType::Grass, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, createVoxel(voxel::VoxelType::Dirt, 1)));
	voxel::RawVolume* rotated = voxel::rotateAxis(&smallVolume, math::Axis::Y);
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	EXPECT_EQ(voxel::VoxelType::Rock, rotated->voxel(0, 0, 0).getMaterial());
	EXPECT_EQ(voxel::VoxelType::Grass, rotated->voxel(0, 1, 0).getMaterial());
	EXPECT_EQ(voxel::VoxelType::Dirt, rotated->voxel(0, 0, 1).getMaterial());
	delete rotated;
}

TEST_F(VolumeRotatorTest, testRotate45Y) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCentre();
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock, 0)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Grass, 0)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Sand, 0)));

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 45, 0), voxel::Voxel(), region.getCentref());
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region& rotatedRegion = rotated->region();
	ASSERT_NE(rotatedRegion, region) << "Rotating by 45 degree should increase the size of the volume "
			<< str(rotatedRegion) << " " << str(region);
}

TEST_F(VolumeRotatorTest, testRotate45YNoExtend) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCentre();
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock, 0)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Grass, 0)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Sand, 0)));

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 45, 0), voxel::Voxel(), region.getCentref(), false);
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region& rotatedRegion = rotated->region();
	ASSERT_EQ(rotatedRegion, region) << "This rotation was forced to not exceed the source bounds "
			<< str(rotatedRegion) << " " << str(region);
	SCOPED_TRACE(str(rotatedRegion) + " " + str(region));
	const glm::ivec3& rotPos = rotatedRegion.getCentre();
	EXPECT_EQ(voxel::VoxelType::Rock, rotated->voxel(rotPos.x, rotPos.y, rotPos.z).getMaterial());
}

TEST_F(VolumeRotatorTest, testRotate90_FourTimes) {
	const voxel::Region region(0, 7);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCentre();
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock, 0)));
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Grass, 0)));
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Sand, 0)));

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 90, 0), voxel::Voxel(), region.getCentref());
	for (int i = 0; i < 3; ++i) {
		voxel::RawVolume* rotated2 = voxel::rotateVolume(rotated, glm::ivec3(0, 90, 0), voxel::Voxel(), region.getCentref());
		ASSERT_NE(nullptr, rotated2) << "No new volume was returned for the desired rotation";
		delete rotated;
		rotated = rotated2;
	}
	const voxel::Region& rotatedRegion = rotated->region();
	EXPECT_EQ(rotatedRegion, region) << "Rotating by 360 degree should increase the size of the volume "
			<< str(rotatedRegion) << " " << str(region);

	EXPECT_EQ(*rotated, smallVolume) << "Expected to get the same volume after 360 degree rotation";
}
}
