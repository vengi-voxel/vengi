/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/MaterialColor.h"
#include "voxel/tests/TestHelper.h"
#include "voxelutil/VolumeRotator.h"

namespace voxelutil {

class VolumeRotatorTest: public app::AbstractTest {
protected:
	inline core::String str(const voxel::Region& region) const {
		return region.toString();
	}
};

TEST_F(VolumeRotatorTest, testRotateAxisZ) {
	const voxel::Region region(-1, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(1)));
	EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(1)));
	voxel::RawVolume* rotated = voxelutil::rotateAxis(&smallVolume, math::Axis::Z);
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 0).getMaterial()) << *rotated;
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(-1, 0, 0).getMaterial()) << *rotated;
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 1, 0).getMaterial()) << *rotated;
	delete rotated;
}

TEST_F(VolumeRotatorTest, testRotateAxisY) {
	const voxel::Region region(-1, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(1)));
	EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(1)));
	voxel::RawVolume* rotated = voxelutil::rotateAxis(&smallVolume, math::Axis::Y);
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 0).getMaterial()) << *rotated;
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 1, 0).getMaterial()) << *rotated;
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, -1).getMaterial()) << *rotated;
	delete rotated;
}

TEST_F(VolumeRotatorTest, testRotateAxisX) {
	const voxel::Region region(-1, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(1)));
	EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(1)));
	voxel::RawVolume* rotated = voxelutil::rotateAxis(&smallVolume, math::Axis::X);
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 0).getMaterial()) << *rotated;
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 1).getMaterial()) << *rotated;
	EXPECT_EQ(voxel::VoxelType::Generic, rotated->voxel(1, 0, 0).getMaterial()) << *rotated;
	delete rotated;
}

TEST_F(VolumeRotatorTest, testRotate45Y) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCenter();
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, voxel::createVoxel(0)));
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, voxel::createVoxel(0)));
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, voxel::createVoxel(0)));

	voxel::RawVolume* rotated = voxelutil::rotateVolume(&smallVolume, glm::ivec3(0, 45, 0), region.getPivot());
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region& rotatedRegion = rotated->region();
	EXPECT_NE(rotatedRegion, region) << "Rotating by 45 degree should increase the size of the volume "
			<< str(rotatedRegion) << " " << str(region);
	delete rotated;
}

}
