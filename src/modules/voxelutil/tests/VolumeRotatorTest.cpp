/**
 * @file
 */

#include "voxelutil/VolumeRotator.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "math/Axis.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxelutil {

class VolumeRotatorTest : public app::AbstractTest {
protected:
	inline core::String str(const voxel::Region &region) const {
		return region.toString();
	}

	voxel::RawVolume *rotateAxis(math::Axis axis, const glm::vec3 &pivot) {
		const voxel::Region region(-1, 1);
		voxel::RawVolume smallVolume(region);
		EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		return voxelutil::rotateAxis(&smallVolume, axis, pivot);
	}
};

TEST_F(VolumeRotatorTest, testRotateAxisX) {
	core::ScopedPtr<voxel::RawVolume> rotated(rotateAxis(math::Axis::X, {0.0f, 0.0f, 0.0f}));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 0).getMaterial()) << *rotated;
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 1).getMaterial()) << *rotated;
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(1, 0, 0).getMaterial()) << *rotated;
}

TEST_F(VolumeRotatorTest, testRotateAxisY) {
	core::ScopedPtr<voxel::RawVolume> rotated(rotateAxis(math::Axis::Y, {0.0f, 0.0f, 0.0f}));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 0).getMaterial()) << *rotated;
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 1, 0).getMaterial()) << *rotated;
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, -1).getMaterial()) << *rotated;
}

TEST_F(VolumeRotatorTest, testRotateAxisZ) {
	core::ScopedPtr<voxel::RawVolume> rotated(rotateAxis(math::Axis::Z, {0.0f, 0.0f, 0.0f}));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 0, 0).getMaterial()) << *rotated;
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(-1, 0, 0).getMaterial()) << *rotated;
	ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(0, 1, 0).getMaterial()) << *rotated;
}

TEST_F(VolumeRotatorTest, testRotateAxisY45) {
	const voxel::Region region(-1, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	core::ScopedPtr<voxel::RawVolume> rotated(
		voxelutil::rotateVolume(&smallVolume, glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(0.0, 0.5f, 0.0f)));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
}

TEST_F(VolumeRotatorTest, DISABLED_testRotate45Y) {
	const voxel::Region region(0, 10);
	voxel::RawVolume smallVolume(region);
	glm::ivec3 pos = region.getCenter();
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(smallVolume.setVoxel(pos.x, pos.y++, pos.z, voxel::createVoxel(voxel::VoxelType::Generic, 0)));

	core::ScopedPtr<voxel::RawVolume> rotated(voxelutil::rotateAxis(&smallVolume, math::Axis::Y));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region &rotatedRegion = rotated->region();
	EXPECT_NE(rotatedRegion, region) << "Rotating by 45 degree should increase the size of the volume "
									 << str(rotatedRegion) << " " << str(region);
}

} // namespace voxelutil
