/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "AbstractVoxelTest.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Voxel.h"

namespace voxel {

class VolumeRotatorTest: public core::AbstractTest {
protected:
	inline std::string str(const voxel::Region& region) const {
		return "mins(" + glm::to_string(region.getLowerCorner()) + "), maxs(" + glm::to_string(region.getUpperCorner()) + ")";
	}

	void rotateY90Steps(int degree) {
		const voxel::Region region(0, 10);
		voxel::RawVolume smallVolume(region);
		glm::ivec3 pos = region.getCentre();
		EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock, 0)));
		EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Grass, 0)));
		EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Sand, 0)));

		voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, degree, 0), voxel::Voxel());
		ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
		const voxel::Region& rotatedRegion = rotated->region();
		ASSERT_EQ(rotatedRegion, region) << "Rotating by " << degree << " degree should not increase the size of the volume "
				<< str(rotatedRegion) << " " << str(region);
		glm::ivec3 rotPos = rotatedRegion.getCentre();
		SCOPED_TRACE(std::to_string(degree) + " degree rotation: " + str(rotatedRegion) + " " + str(region));
		EXPECT_EQ(voxel::VoxelType::Rock, rotated->voxel(rotPos.x, rotPos.y++, rotPos.z).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Grass, rotated->voxel(rotPos.x, rotPos.y++, rotPos.z).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Sand, rotated->voxel(rotPos.x, rotPos.y++, rotPos.z).getMaterial());
	}
};

TEST_F(VolumeRotatorTest, testRotateY90Steps) {
	rotateY90Steps(0);
	rotateY90Steps(90);
	rotateY90Steps(180);
	rotateY90Steps(270);
	rotateY90Steps(360);
	rotateY90Steps(720);
}

TEST_F(VolumeRotatorTest, testRotate45Y) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCentre();
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Rock, 0)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Grass, 0)));
	ASSERT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, createVoxel(voxel::VoxelType::Sand, 0)));

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 45, 0), voxel::Voxel());
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

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 45, 0), voxel::Voxel(), false);
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

	voxel::RawVolume* rotated = voxel::rotateVolume(&smallVolume, glm::ivec3(0, 90, 0), voxel::Voxel());
	for (int i = 0; i < 3; ++i) {
		voxel::RawVolume* rotated2 = voxel::rotateVolume(rotated, glm::ivec3(0, 90, 0), voxel::Voxel());
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
