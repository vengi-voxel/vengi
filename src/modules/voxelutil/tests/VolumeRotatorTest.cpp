/**
 * @file
 */

#include "voxelutil/VolumeRotator.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "math/Axis.h"
#include "math/Math.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/euler_angles.hpp>

namespace voxelutil {

class VolumeRotatorTest : public app::AbstractTest {
protected:
	// {0, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}
	void rotateAxisAndValidate(math::Axis axis, const glm::ivec3 positions[4]) {
		const voxel::Region region(-1, 1);
		voxel::RawVolume smallVolume(region);
		// build a block in a 3x3 volume area with the center voxel
		// being at 0,0,0
		// -------
		// |  x  |
		// |x c x|
		// |     |
		// -------
		EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(-1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		core::ScopedPtr<voxel::RawVolume> rotated(voxelutil::rotateAxis(&smallVolume, axis));
		ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
		ASSERT_EQ(rotated->region(), region) << "Rotating around an axis should not modify the region";
		for (int i = 0; i < 4; ++i) {
			EXPECT_TRUE(voxel::isBlocked(rotated->voxel(positions[i]).getMaterial()))
				<< "Expected to find a voxel at position " << positions[i] << "(" << i << ")\n"
				<< *rotated;
		}
		glm::ivec3 remaining(0);
		remaining[math::getIndexForAxis(axis)] = 270;

		core::ScopedPtr<voxel::RawVolume> unRotated;
		while (remaining.x >= 90 || remaining.y >= 90 || remaining.z >= 90) {
			const voxel::RawVolume *input = unRotated == nullptr ? rotated : unRotated;
			math::Axis remainingAxis;
			if (remaining.x >= 90) {
				remainingAxis = math::Axis::X;
				remaining.x -= 90;
			} else if (remaining.y >= 90) {
				remainingAxis = math::Axis::Y;
				remaining.y -= 90;
			} else {
				core_assert(remaining.z >= 90);
				remainingAxis = math::Axis::Z;
				remaining.z -= 90;
			}

			unRotated = rotateAxis(input, remainingAxis);
		}
		ASSERT_EQ(unRotated->region(), region);
		ASSERT_TRUE(voxel::isBlocked(unRotated->voxel(0, 0, 0).getMaterial())) << "Expected to find a voxel\n"
																			   << smallVolume << "\nversus\n"
																			   << *unRotated;
		ASSERT_TRUE(voxel::isBlocked(unRotated->voxel(0, 1, 0).getMaterial())) << "Expected to find a voxel\n"
																			   << smallVolume << "\nversus\n"
																			   << *unRotated;
		ASSERT_TRUE(voxel::isBlocked(unRotated->voxel(-1, 0, 0).getMaterial())) << "Expected to find a voxel\n"
																			   << smallVolume << "\nversus\n"
																			   << *unRotated;
	}
};

TEST_F(VolumeRotatorTest, testRotateAxisX) {
	const math::Axis axis = math::Axis::X;
	const glm::ivec3 positions[]{{0, 0, 0}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0}};
	rotateAxisAndValidate(axis, positions);
}

TEST_F(VolumeRotatorTest, testRotateAxisY) {
	const math::Axis axis = math::Axis::Y;
	const glm::ivec3 positions[]{{0, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, -1}};
	rotateAxisAndValidate(axis, positions);
}

TEST_F(VolumeRotatorTest, testRotateAxisZ) {
	const math::Axis axis = math::Axis::Z;
	const glm::ivec3 positions[]{{0, 0, 0}, {0, 1, 0}, {0, -1, 0}, {1, 0, 0}};
	rotateAxisAndValidate(axis, positions);
}

TEST_F(VolumeRotatorTest, testRotateAxisY45) {
	const voxel::Region region(-1, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	const glm::vec3 angles(0.0f, 45.0f, 0.0f);
	const glm::vec3 pivot(0.0f, 0.5f, 0.0f);
	core::ScopedPtr<voxel::RawVolume> rotated(voxelutil::rotateVolume(&smallVolume, angles, pivot));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
	const voxel::Region &rotatedRegion = rotated->region();
	EXPECT_NE(rotatedRegion, region) << "Rotating by 45 degree should increase the size of the volume " << rotatedRegion
									 << " " << region;
}

TEST_F(VolumeRotatorTest, testRotateXYZByAngle) {
	const voxel::Region region(-1, 6);
	voxel::RawVolume smallVolume(region);

	const glm::ivec3 originalPositions[] = {
		glm::ivec3(0, 0, 0),
		glm::ivec3(0, 1, 0),
		glm::ivec3(1, 0, 0),
		glm::ivec3(5, 3, 0),
		glm::ivec3(2, 5, 0)
	};

	for (const glm::ivec3 &p : originalPositions) {
		EXPECT_TRUE(smallVolume.setVoxel(p, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	}
	const glm::vec3 angles(13.0f, 8.0f, 70.0f);
	const glm::vec3 pivot(0.0f, 0.0f, 0.0f);
	core::ScopedPtr<voxel::RawVolume> rotated(voxelutil::rotateVolume(&smallVolume, angles, pivot));
	ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";

	// Validate the rotated volume voxels being at the expected position by rotating
	// the three setVoxel coordinates by the given angles and pivot and then checking
	// the voxels in the volume
	const float pitch = glm::radians(angles.x);
	const float yaw = glm::radians(angles.y);
	const float roll = glm::radians(angles.z);
	const glm::mat4 rotationMatrix = glm::eulerAngleXYZ(pitch, yaw, roll);

	const glm::vec3 actualPivot(pivot * glm::vec3(region.getDimensionsInVoxels()));

	for (const glm::ivec3 &p : originalPositions) {
		const glm::ivec3 expectedPos = math::transform(rotationMatrix, p, actualPivot);
		EXPECT_TRUE(voxel::isBlocked(rotated->voxel(expectedPos).getMaterial()))
			<< "Expected to find a voxel at position " << expectedPos
			<< " (transformed from " << p << ")\n"
			<< *rotated;
	}
}

TEST_F(VolumeRotatorTest, testMirrorAxisX) {
	const voxel::Region region(-1, -1, -1, 2, 2, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, -1, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	core::ScopedPtr<voxel::RawVolume> mirrored(voxelutil::mirrorAxis(&smallVolume, math::Axis::X));
	ASSERT_NE(nullptr, mirrored) << "No new volume was returned for the desired mirroring";
	ASSERT_EQ(mirrored->region(), region) << "Mirroring around an axis should not modify the region";
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(1, 0, 0).getMaterial())) << smallVolume << *mirrored;
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(1, 1, 0).getMaterial())) << smallVolume << *mirrored;
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(1, 0, -1).getMaterial())) << smallVolume << *mirrored;
}

TEST_F(VolumeRotatorTest, testMirrorAxisY) {
	const voxel::Region region(-1, -1, -1, 2, 2, 1);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, -1, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	core::ScopedPtr<voxel::RawVolume> mirrored(voxelutil::mirrorAxis(&smallVolume, math::Axis::Y));
	ASSERT_NE(nullptr, mirrored) << "No new volume was returned for the desired mirroring";
	ASSERT_EQ(mirrored->region(), region) << "Mirroring around an axis should not modify the region";
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(0, 0, 0).getMaterial())) << smallVolume << *mirrored;
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(0, 1, 0).getMaterial())) << smallVolume << *mirrored;
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(0, 1, -1).getMaterial())) << smallVolume << *mirrored;
}

TEST_F(VolumeRotatorTest, testMirrorAxisZ) {
	const voxel::Region region(-1, -1, -1, 1, 2, 2);
	voxel::RawVolume smallVolume(region);
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(smallVolume.setVoxel(0, 0, -1, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	core::ScopedPtr<voxel::RawVolume> mirrored(voxelutil::mirrorAxis(&smallVolume, math::Axis::Z));
	ASSERT_NE(nullptr, mirrored) << "No new volume was returned for the desired mirroring";
	ASSERT_EQ(mirrored->region(), region) << "Mirroring around an axis should not modify the region";
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(0, 0, 1).getMaterial())) << smallVolume << *mirrored;
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(0, 1, 1).getMaterial())) << smallVolume << *mirrored;
	EXPECT_TRUE(voxel::isBlocked(mirrored->voxel(0, 0, 2).getMaterial())) << smallVolume << *mirrored;
}

} // namespace voxelutil
