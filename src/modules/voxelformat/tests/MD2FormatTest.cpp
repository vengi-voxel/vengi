/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class MD2FormatTest : public AbstractFormatTest {};

TEST_F(MD2FormatTest, testVoxelize) {
	// public domain model from https://github.com/ufoaiorg/ufoai/blob/master/base/models/objects/barrel_fuel/barrel_fuel.md2 (Nobiax/yughues, Open Game Art (http://openga)
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "fuel_can.md2", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::Region &region = node->region();
	EXPECT_EQ(region.getLowerX(), 0);
	EXPECT_EQ(region.getLowerY(), 0);
	EXPECT_EQ(region.getLowerZ(), 0);
	EXPECT_EQ(region.getUpperX(), 25);
	EXPECT_EQ(region.getUpperY(), 32);
	EXPECT_EQ(region.getUpperZ(), 25);
	const int cntVoxels = voxelutil::countVoxels(*node->volume());
	EXPECT_EQ(cntVoxels, 12158);
}

} // namespace voxelformat
