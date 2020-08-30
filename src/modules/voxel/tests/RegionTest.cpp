/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/Region.h"

namespace voxel {

class RegionTest: public app::AbstractTest {
};

TEST_F(RegionTest, testContains) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	ASSERT_TRUE(region.containsPoint(mins));
	ASSERT_TRUE(region.containsPoint(maxs));
	ASSERT_FALSE(region.containsPoint(mins, 1));
	ASSERT_FALSE(region.containsPoint(maxs, 1));
	ASSERT_FALSE(region.containsPoint(maxs + 1));
	ASSERT_TRUE(region.containsRegion(region));
	ASSERT_FALSE(region.containsRegion(region, 1));
}

TEST_F(RegionTest, testMoveIntoRegionSize1WithOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(0, 0, 0);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::zero<glm::ivec3>());
}

TEST_F(RegionTest, testMoveIntoRegionSize1NoOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(0, 0, 0);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(0, 0, 0);
	ASSERT_EQ(pos, glm::zero<glm::ivec3>());
}

TEST_F(RegionTest, testMoveIntoRegionSize1XOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(0, 0, 0);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(10, 0, 0);
	ASSERT_EQ(pos, glm::zero<glm::ivec3>());
}

TEST_F(RegionTest, testMoveIntoNoOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(2));
}

TEST_F(RegionTest, testMoveIntoYOverlap) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(2, 20, 2);
	ASSERT_EQ(pos, glm::ivec3(2, 9, 2));
}

TEST_F(RegionTest, testMoveIntoYBoundary) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(2, maxs.y, 2);
	ASSERT_EQ(pos, glm::ivec3(2, maxs.y, 2));
}

TEST_F(RegionTest, testMoveIntoYBoundaryNoOriginZero) {
	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(11, 11, 11);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(10, 10, 10));
}

TEST_F(RegionTest, testMoveIntoYBoundaryNoOriginZeroNoOverlap) {
	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(12, 12, 12));
}

TEST_F(RegionTest, testMoveIntoNegativeMins) {
	const glm::ivec3 mins(-10, -10, -10);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(2, 2, 2);
	ASSERT_EQ(pos, glm::ivec3(-8, -8, -8));
}

TEST_F(RegionTest, testMoveIntoNegativeSteps) {
	const glm::ivec3 mins(-10, -10, -10);
	const glm::ivec3 maxs(15, 15, 15);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(-2, -2, -2);
	ASSERT_EQ(pos, glm::ivec3(13, 13, 13));
}

TEST_F(RegionTest, testMoveIntoBiggerThanSize) {
	const glm::ivec3 mins(-10, -10, -10);
	const glm::ivec3 maxs(10, 10, 10);
	voxel::Region region(mins, maxs);
	const glm::ivec3& pos = region.moveInto(41, 41, -41);
	ASSERT_EQ(pos, glm::ivec3(10, 10, -10));
}

}
