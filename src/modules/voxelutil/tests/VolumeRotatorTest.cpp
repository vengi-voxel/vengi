/**
 * @file
 */

#include "voxelutil/VolumeRotator.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "math/Axis.h"
#include "math/Math.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const ivec3 &v) {
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}
::std::ostream &operator<<(::std::ostream &os, const vec3 &v) {
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}
} // namespace glm

namespace voxelutil {

class VolumeRotatorTest : public app::AbstractTest {
protected:
	void rotateAxisAndValidate(math::Axis axis, const glm::ivec3 positions[3]) {
		const voxel::Region region(0, 2);
		voxel::RawVolume smallVolume(region);
		EXPECT_TRUE(smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_TRUE(smallVolume.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		core::ScopedPtr<voxel::RawVolume> rotated(voxelutil::rotateAxis(&smallVolume, axis));
		ASSERT_NE(nullptr, rotated) << "No new volume was returned for the desired rotation";
		ASSERT_EQ(rotated->region(), region) << "Rotating around an axis should not modify the region";
		for (int i = 0; i < 3; ++i) {
			ASSERT_TRUE(voxel::isBlocked(rotated->voxel(positions[i]).getMaterial()))
				<< "Expected to find a voxel at position " << positions[i] << "\n"
				<< smallVolume << "\nversus\n"
				<< *rotated;
		}
		glm::ivec3 angles(0);
		angles[math::getIndexForAxis(axis)] = 270;
		core::ScopedPtr<voxel::RawVolume> unRotated(voxelutil::rotateVolume(rotated, angles, glm::vec3(0.0f)));
		ASSERT_NE(nullptr, unRotated);
		ASSERT_EQ(unRotated->region(), region);
		ASSERT_TRUE(voxel::isBlocked(unRotated->voxel(0, 0, 0).getMaterial())) << "Expected to find a voxel\n"
																			   << smallVolume << "\nversus\n"
																			   << *unRotated;
		ASSERT_TRUE(voxel::isBlocked(unRotated->voxel(0, 1, 0).getMaterial())) << "Expected to find a voxel\n"
																			   << smallVolume << "\nversus\n"
																			   << *unRotated;
		ASSERT_TRUE(voxel::isBlocked(unRotated->voxel(1, 0, 0).getMaterial())) << "Expected to find a voxel\n"
																			   << smallVolume << "\nversus\n"
																			   << *unRotated;
	}
};

TEST_F(VolumeRotatorTest, testRotateAxisX) {
	const math::Axis axis = math::Axis::X;
	const glm::ivec3 positions[]{{1, 0, 2}, {0, 0, 2}, {0, 0, 1}};
	rotateAxisAndValidate(axis, positions);
}

TEST_F(VolumeRotatorTest, testRotateAxisY) {
	const math::Axis axis = math::Axis::Y;
	const glm::ivec3 positions[]{{2, 0, 0}, {2, 0, 1}, {2, 1, 0}};
	rotateAxisAndValidate(axis, positions);
}

TEST_F(VolumeRotatorTest, testRotateAxisZ) {
	const math::Axis axis = math::Axis::Z;
	const glm::ivec3 positions[]{{1, 2, 0}, {0, 2, 0}, {0, 1, 0}};
	rotateAxisAndValidate(axis, positions);
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
	const voxel::Region &rotatedRegion = rotated->region();
	EXPECT_NE(rotatedRegion, region) << "Rotating by 45 degree should increase the size of the volume " << rotatedRegion
									 << " " << region;
}

} // namespace voxelutil
