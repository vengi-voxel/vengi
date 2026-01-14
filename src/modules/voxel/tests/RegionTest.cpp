/**
 * @file
 */

#include "voxel/Region.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include <glm/ext/scalar_constants.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/euler_angles.hpp>

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
		EXPECT_EQ(region.getLowerY(), mins.y) << "The rotated volume should be at the same height as the original one";
		EXPECT_EQ(region.getUpperY(), maxs.y) << "The rotated volume should be at the same height as the original one";
	}
};

TEST_F(RegionTest, testContains) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	ASSERT_TRUE(region.containsPoint(mins));
	ASSERT_TRUE(region.containsPoint({mins.x, mins.y, mins.z, 0}));
	ASSERT_TRUE(region.containsPoint(maxs));
	ASSERT_TRUE(region.containsPoint({maxs.x, maxs.y, maxs.z, 0}));
	ASSERT_FALSE(region.containsPoint(maxs + 1));
	ASSERT_FALSE(region.containsPoint({maxs.x + 1, maxs.y + 1, maxs.z + 1, 0}));
	ASSERT_TRUE(region.containsRegion(region));
}

TEST_F(RegionTest, testRotateAxisY45) {
	const glm::mat4 &mat = glm::eulerAngleY(glm::radians(45.0f));
	const voxel::Region region(0, 3);
	const voxel::Region &rotated = region.rotate(mat, region.calcCellCenterf());
	const glm::ivec3 mins = rotated.getLowerCorner();
	const glm::ivec3 maxs = rotated.getUpperCorner();

	EXPECT_EQ(0, mins.y) << "The rotated volume should be at the same height as the original one";
	EXPECT_EQ(3, maxs.y) << "The rotated volume should be at the same height as the original one";
	EXPECT_EQ(-1, mins.x);
	EXPECT_EQ(4, maxs.x);
	EXPECT_EQ(-1, mins.z);
	EXPECT_EQ(4, maxs.z);
}

TEST_F(RegionTest, testRotateAxisY90) {
	const glm::mat4 &mat = glm::eulerAngleY(glm::radians(90.0f));
	const voxel::Region region(-10, 10);
	const glm::vec3 center(0.0f);
	const glm::ivec3 dimensionBeforeRotation = region.getDimensionsInVoxels();
	const voxel::Region &rotated = region.rotate(mat, center);
	const glm::ivec3 mins = rotated.getLowerCorner();
	const glm::ivec3 maxs = rotated.getUpperCorner();
	const glm::ivec3 dimensionAfterRotation = rotated.getDimensionsInVoxels();

	EXPECT_EQ(-10, mins.y) << "The rotated volume should be at the same height as the original one";
	EXPECT_EQ(10, maxs.y) << "The rotated volume should be at the same height as the original one";
	EXPECT_EQ(-10, mins.x);
	EXPECT_EQ(10, maxs.x);
	EXPECT_EQ(-10, mins.z);
	EXPECT_EQ(10, maxs.z);

	EXPECT_EQ(dimensionBeforeRotation, dimensionAfterRotation);
}

TEST_F(RegionTest, testCrop) {
	voxel::Region region1(-2, -2, -2, 65, 65, 65);
	voxel::Region region2(0, 0, 68, 31, 31, 99);
	EXPECT_FALSE(region1.cropTo(region2));
	EXPECT_TRUE(region1.cropTo(region1));
}

TEST_F(RegionTest, testRotateAxisPivotMins) {
	const voxel::Region region(-10, 10);
	rotateAroundPivot(region, region.getLowerCornerf());
}

TEST_F(RegionTest, testRotateAxisPivotMaxs) {
	const voxel::Region region(-10, 10);
	rotateAroundPivot(region, region.getUpperCornerf());
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

TEST_F(RegionTest, testDimensions) {
	voxel::Region region(0, 3);
	EXPECT_EQ(glm::ivec3(4), region.getDimensionsInVoxels());
	EXPECT_EQ(glm::ivec3(3), region.getDimensionsInCells());
}

TEST_F(RegionTest, testCenter) {
	voxel::Region region(0, 3);
	EXPECT_EQ(glm::ivec3(1), region.getCenter());
	EXPECT_EQ(glm::vec3(2.0f), region.calcCenterf());

	voxel::Region region2(-1, 1);
	EXPECT_EQ(glm::ivec3(0), region2.getCenter());
	EXPECT_EQ(glm::vec3(0.5f), region2.calcCenterf());

	voxel::Region region3(-2, 11);
	EXPECT_EQ(glm::ivec3(4), region3.getCenter());
	EXPECT_VEC_NEAR(glm::vec3(5.0f), region3.calcCenterf(), glm::epsilon<float>());

	voxel::Region region4(0, 0);
	EXPECT_EQ(glm::ivec3(0), region4.getCenter());
	EXPECT_VEC_NEAR(glm::vec3(0.5f), region4.calcCenterf(), glm::epsilon<float>());
}

TEST_F(RegionTest, testSubtract) {
	voxel::Region a(0, 3);
	voxel::Region b(1, 1);
	const core::Buffer<voxel::Region> &remainingSelections = voxel::Region::subtract(a, b);
	EXPECT_EQ(6u, remainingSelections.size());
	for (const voxel::Region &region : remainingSelections) {
		EXPECT_FALSE(intersects(b, region));
		EXPECT_TRUE(a.containsRegion(region));
	}
}

TEST_F(RegionTest, testIndex) {
	voxel::Region region(1, 3);
	EXPECT_EQ(0, region.index(1, 1, 1));
	EXPECT_EQ(1, region.index(2, 1, 1));
	EXPECT_EQ(2, region.index(3, 1, 1));
	EXPECT_EQ(3, region.index(1, 2, 1));
	EXPECT_EQ(4, region.index(2, 2, 1));
	EXPECT_EQ(5, region.index(3, 2, 1));
	EXPECT_EQ(6, region.index(1, 3, 1));
	EXPECT_EQ(26, region.index(3, 3, 3));
}

TEST_F(RegionTest, testIndexBackAndForth) {
	voxel::Region region(1, 3);
	const int size = region.voxels();
	for (int i = 0; i < size; ++i) {
		const glm::ivec3 pos = region.fromIndex(i);
		const int idx = region.index(pos);
		EXPECT_EQ(i, idx);
	}
}

TEST_F(RegionTest, testSubtractExtendsOutside) {
	voxel::Region a(0, 10);
	voxel::Region b(0, 0, -5, 10, 5, 15);
	const core::Buffer<voxel::Region> &remainingSelections = voxel::Region::subtract(a, b);
	EXPECT_EQ(1u, remainingSelections.size());
	if (!remainingSelections.empty()) {
		EXPECT_EQ(voxel::Region(0, 6, 0, 10, 10, 10), remainingSelections[0]);
	}
}

} // namespace voxel
