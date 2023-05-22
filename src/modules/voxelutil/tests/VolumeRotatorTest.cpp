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
	void validate(const core::ScopedPtr<voxel::RawVolume> &rotated, int x, int y, int z) {
		ASSERT_EQ(voxel::VoxelType::Generic, rotated->voxel(x, y, z).getMaterial()) << *rotated;
	}

	voxel::RawVolume *rotateAxis(math::Axis axis, const glm::vec3 &pivot) {
		const voxel::Region region(-1, 1);
		voxel::RawVolume smallVolume(region);
		EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		voxel::RawVolume *rotated = voxelutil::rotateAxis(&smallVolume, axis, pivot);
		if (rotated) {
			EXPECT_EQ(rotated->region(), region);
		}
		return rotated;
	}

	void rotateAxisAndValidate(math::Axis axis, const glm::vec3 &pivot, const glm::ivec3 positions[3]) {
		core::ScopedPtr<voxel::RawVolume> rotated(rotateAxis(axis, pivot));
		ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
		validate(rotated, positions[0].x, positions[0].y, positions[0].z);
		validate(rotated, positions[1].x, positions[1].y, positions[1].z);
		validate(rotated, positions[2].x, positions[2].y, positions[2].z);
	}
};

TEST_F(VolumeRotatorTest, testRotateAxisX) {
	const math::Axis axis = math::Axis::X;
	const glm::vec3 pivot{0.0f, 0.0f, 0.0f};
	const glm::ivec3 positions[] = {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}};
	rotateAxisAndValidate(axis, pivot, positions);
}

TEST_F(VolumeRotatorTest, testRotateAxisY) {
	const math::Axis axis = math::Axis::Y;
	const glm::vec3 pivot{0.0f, 0.0f, 0.0f};
	const glm::ivec3 positions[] = {{0, 0, 0}, {0, 1, 0}, {0, 0, -1}};
	rotateAxisAndValidate(axis, pivot, positions);
}

TEST_F(VolumeRotatorTest, testRotateAxisZ) {
	const math::Axis axis = math::Axis::Z;
	const glm::vec3 pivot{0.0f, 0.0f, 0.0f};
	const glm::ivec3 positions[] = {{0, 0, 0}, {-1, 0, 0}, {0, 1, 0}};
	rotateAxisAndValidate(axis, pivot, positions);
}

TEST_F(VolumeRotatorTest, DISABLED_testRotateAxisXPivot) {
	const math::Axis axis = math::Axis::X;
	const glm::vec3 pivot{0.5f, 0.5f, 0.5f};
	const glm::ivec3 positions[] = {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}};
	rotateAxisAndValidate(axis, pivot, positions);
}

TEST_F(VolumeRotatorTest, DISABLED_testRotateAxisYPivot) {
	const math::Axis axis = math::Axis::Y;
	const glm::vec3 pivot{0.5f, 0.5f, 0.5f};
	const glm::ivec3 positions[] = {{0, 0, 0}, {0, 1, 0}, {0, 0, -1}};
	rotateAxisAndValidate(axis, pivot, positions);
}

TEST_F(VolumeRotatorTest, DISABLED_testRotateAxisZPivot) {
	const math::Axis axis = math::Axis::Z;
	const glm::vec3 pivot{0.5f, 0.5f, 0.5f};
	const glm::ivec3 positions[] = {{0, 0, 0}, {-1, 0, 0}, {0, 1, 0}};
	rotateAxisAndValidate(axis, pivot, positions);
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
	EXPECT_NE(rotatedRegion, region) << "Rotating by 45 degree should increase the size of the volume " << rotatedRegion
									 << " " << region;
}

} // namespace voxelutil
