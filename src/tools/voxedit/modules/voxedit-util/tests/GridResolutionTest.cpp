/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/Face.h"

namespace voxedit {

class GridResolutionTest : public app::AbstractTest {};

TEST_F(GridResolutionTest, testParseUniform) {
	EXPECT_EQ(glm::ivec3(1), parseGridResolution("1"));
	EXPECT_EQ(glm::ivec3(3), parseGridResolution("3"));
	EXPECT_EQ(glm::ivec3(3), parseGridResolution("3 3 3"));
}

TEST_F(GridResolutionTest, testParseNonUniform) {
	EXPECT_EQ(glm::ivec3(1, 2, 3), parseGridResolution("1 2 3"));
	EXPECT_EQ(glm::ivec3(1, 2, 3), parseGridResolution("1:2:3"));
	EXPECT_EQ(glm::ivec3(1, 2, 3), parseGridResolution("1,2,3"));
}

TEST_F(GridResolutionTest, testParseClamp) {
	EXPECT_EQ(glm::ivec3(1), parseGridResolution("0"));
	EXPECT_EQ(glm::ivec3(64), parseGridResolution("100"));
	EXPECT_EQ(glm::ivec3(1, 64, 2), parseGridResolution("0 100 2"));
}

TEST_F(GridResolutionTest, testParseEmpty) {
	EXPECT_EQ(glm::ivec3(1), parseGridResolution(""));
}

class TestShapeBrush : public ShapeBrush {
public:
	using ShapeBrush::calcRegion;
	using AABBBrush::applyGridResolution;
};

TEST_F(GridResolutionTest, testApplyGridResolutionPerAxis) {
	TestShapeBrush brush;
	ASSERT_TRUE(brush.init());
	const glm::ivec3 snapped = brush.applyGridResolution(glm::ivec3(5, 7, 11), glm::ivec3(2, 3, 4));
	EXPECT_EQ(glm::ivec3(4, 6, 8), snapped);
	brush.shutdown();
}

TEST_F(GridResolutionTest, testCalcRegionNonUniform) {
	TestShapeBrush brush;
	ASSERT_TRUE(brush.init());
	BrushContext ctx;
	ctx.gridResolution = glm::ivec3(1, 2, 3);
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ASSERT_TRUE(brush.beginBrush(ctx));
	const voxel::Region region = brush.calcRegion(ctx);
	EXPECT_EQ(glm::ivec3(1, 2, 3), region.getDimensionsInVoxels());
	EXPECT_EQ(glm::ivec3(0, 0, 0), region.getLowerCorner());
	EXPECT_EQ(glm::ivec3(0, 1, 2), region.getUpperCorner());
	brush.endBrush(ctx);
	brush.shutdown();
}

} // namespace voxedit
