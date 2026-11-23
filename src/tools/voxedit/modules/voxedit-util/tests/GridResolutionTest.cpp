/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/Face.h"
#include "math/Vec3i.h"

namespace voxedit {

class GridResolutionTest : public app::AbstractTest {};

TEST_F(GridResolutionTest, testParseUniform) {
	EXPECT_EQ(math::Vec3i(1), parseGridResolution("1"));
	EXPECT_EQ(math::Vec3i(3), parseGridResolution("3"));
	EXPECT_EQ(math::Vec3i(3), parseGridResolution("3 3 3"));
}

TEST_F(GridResolutionTest, testParseNonUniform) {
	EXPECT_EQ(math::Vec3i(1, 2, 3), parseGridResolution("1 2 3"));
	EXPECT_EQ(math::Vec3i(1, 2, 3), parseGridResolution("1:2:3"));
	EXPECT_EQ(math::Vec3i(1, 2, 3), parseGridResolution("1,2,3"));
}

TEST_F(GridResolutionTest, testParseClamp) {
	EXPECT_EQ(math::Vec3i(1), parseGridResolution("0"));
	EXPECT_EQ(math::Vec3i(64), parseGridResolution("100"));
	EXPECT_EQ(math::Vec3i(1, 64, 2), parseGridResolution("0 100 2"));
}

TEST_F(GridResolutionTest, testParseEmpty) {
	EXPECT_EQ(math::Vec3i(1), parseGridResolution(""));
}

class TestShapeBrush : public ShapeBrush {
public:
	using ShapeBrush::calcRegion;
	using AABBBrush::applyGridResolution;
};

TEST_F(GridResolutionTest, testApplyGridResolutionPerAxis) {
	TestShapeBrush brush;
	ASSERT_TRUE(brush.init());
	const math::Vec3i snapped = brush.applyGridResolution(math::Vec3i(5, 7, 11), math::Vec3i(2, 3, 4));
	EXPECT_EQ(math::Vec3i(4, 6, 8), snapped);
	brush.shutdown();
}

TEST_F(GridResolutionTest, testCalcRegionNonUniform) {
	TestShapeBrush brush;
	ASSERT_TRUE(brush.init());
	BrushContext ctx;
	ctx.gridResolution = math::Vec3i(1, 2, 3);
	ctx.cursorPosition = math::Vec3i(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ASSERT_TRUE(brush.beginBrush(ctx));
	const voxel::Region region = brush.calcRegion(ctx);
	EXPECT_EQ(math::Vec3i(1, 2, 3), region.getDimensionsInVoxels());
	EXPECT_EQ(math::Vec3i(0, 0, 0), region.getLowerCorner());
	EXPECT_EQ(math::Vec3i(0, 1, 2), region.getUpperCorner());
	brush.endBrush(ctx);
	brush.shutdown();
}

} // namespace voxedit
