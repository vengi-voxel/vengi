/**
 * @file
 */

#include "voxel/Region.h"
#include "app/tests/AbstractTest.h"
#include <glm/gtx/euler_angles.hpp>

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const ivec3 &v) {
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}
} // namespace glm

namespace voxel {

class RegionTest : public app::AbstractTest {
public:
	void rotateAroundPivot(const voxel::Region &region, const glm::vec3 &pivot) {
		const glm::mat4 &mat = glm::eulerAngleY(glm::radians(90.0f));
		const glm::ivec3 dimensions = region.getDimensionsInVoxels();
		const voxel::Region &rotated = region.rotate(mat, pivot);
		EXPECT_EQ(dimensions, rotated.getDimensionsInVoxels());
		const glm::ivec3 mins = rotated.getLowerCorner();
		const glm::ivec3 maxs = rotated.getUpperCorner();
		EXPECT_EQ(-10, mins.y) << "The rotated volume should be at the same height as the original one";
		EXPECT_EQ(10, maxs.y) << "The rotated volume should be at the same height as the original one";
	}
};

TEST_F(RegionTest, testContains) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	ASSERT_TRUE(region.containsPoint(mins));
	ASSERT_TRUE(region.containsPoint(maxs));
	ASSERT_FALSE(region.containsPoint(maxs + 1));
	ASSERT_TRUE(region.containsRegion(region));
}

TEST_F(RegionTest, testRotateAxisY45) {
	const glm::vec3 angles(0.0f, 45.0f, 0.0f);
	const float pitch = glm::radians(angles.x);
	const float yaw = glm::radians(angles.y);
	const float roll = glm::radians(angles.z);
	const glm::mat4 &mat = glm::eulerAngleXYZ(pitch, yaw, roll);
	const glm::vec3 pivot(0.0f, 0.0f, 0.0f);

	const voxel::Region region(-10, 10);

	const voxel::Region &rotated = region.rotate(mat, pivot);
	const glm::ivec3 mins = rotated.getLowerCorner();
	const glm::ivec3 maxs = rotated.getUpperCorner();

	EXPECT_EQ(-10, mins.y) << "The rotated volume should be at the same height as the original one";
	EXPECT_EQ(10, maxs.y) << "The rotated volume should be at the same height as the original one";
	EXPECT_EQ(-14, mins.x);
	EXPECT_EQ(14, maxs.x);
	EXPECT_EQ(-15, mins.z);
	EXPECT_EQ(14, maxs.z);
}

TEST_F(RegionTest, testRotateAxisPivotMins) {
	const voxel::Region region(-10, 10);
	const glm::vec3 pivot(-10.0f, -10.0f, -10.0f);
	rotateAroundPivot(region, pivot);
}

TEST_F(RegionTest, testRotateAxisPivotMaxs) {
	const voxel::Region region(-10, 10);
	const glm::vec3 pivot(10.0f, 10.0f, 10.0f);
	rotateAroundPivot(region, pivot);
}

TEST_F(RegionTest, testRotateAxisPivotMinsxy) {
	const voxel::Region region(-10, 10);
	const glm::vec3 pivot(-10.0f, -10.0f, 10.0f);
	rotateAroundPivot(region, pivot);
}

TEST_F(RegionTest, testRotateAxisPivotMaxsxy) {
	const voxel::Region region(-10, 10);
	const glm::vec3 pivot(10.0f, 10.0f, -10.0f);
	rotateAroundPivot(region, pivot);
}

TEST_F(RegionTest, testMoveIntoRegionSize1WithOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(0, 0, 0);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(0));
}

TEST_F(RegionTest, testMoveIntoRegionSize1NoOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(0, 0, 0);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(0, 0, 0);
	ASSERT_EQ(pos, glm::ivec3(0));
}

TEST_F(RegionTest, testMoveIntoRegionSize1XOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(0, 0, 0);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(10, 0, 0);
	ASSERT_EQ(pos, glm::ivec3(0));
}

TEST_F(RegionTest, testMoveIntoNoOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(2));
}

TEST_F(RegionTest, testMoveIntoYOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(2, 20, 2);
	ASSERT_EQ(pos, glm::ivec3(2, 9, 2));
}

TEST_F(RegionTest, testMoveIntoYBoundary) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(2, maxs.y, 2);
	ASSERT_EQ(pos, glm::ivec3(2, maxs.y, 2));
}

TEST_F(RegionTest, testMoveIntoYBoundaryNoOriginZero) {
	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(11, 11, 11);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(10, 10, 10));
}

TEST_F(RegionTest, testMoveIntoYBoundaryNoOriginZeroNoOverlap) {
	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(12, 12, 12));
}

TEST_F(RegionTest, testMoveIntoNegativeMins) {
	const glm::ivec3 mins(-10, -10, -10);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(-8, -8, -8));
}

TEST_F(RegionTest, testMoveIntoNegativeSteps) {
	const glm::ivec3 mins(-10, -10, -10);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(-2, -2, -2);
	ASSERT_EQ(pos, glm::ivec3(13, 13, 13));
}

TEST_F(RegionTest, testMoveIntoBiggerThanSize) {
	const glm::ivec3 mins(-10, -10, -10);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3 &pos = region.moveInto(41, 41, -41);
	ASSERT_EQ(pos, glm::ivec3(10, 10, -10));
}

} // namespace voxel
