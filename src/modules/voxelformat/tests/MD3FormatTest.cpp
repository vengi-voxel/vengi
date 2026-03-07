/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

class MD3FormatTest : public AbstractFormatTest {};

TEST_F(MD3FormatTest, testVoxelize) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-md3.md3", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::Region &region = node->region();
	EXPECT_GE(region.getWidthInVoxels(), 1);
	EXPECT_GE(region.getHeightInVoxels(), 1);
	EXPECT_GE(region.getDepthInVoxels(), 1);
}

} // namespace voxelformat
