/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

class Autodesk3DSFormatTest : public AbstractFormatTest {};

TEST_F(Autodesk3DSFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "gun.3ds", 2);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ("cube5", node->name());
	EXPECT_NE(nullptr, node->volume());
	EXPECT_EQ(4, node->volume()->region().getWidthInVoxels());
	EXPECT_EQ(5, node->volume()->region().getHeightInVoxels());
	EXPECT_EQ(4, node->volume()->region().getDepthInVoxels());
	EXPECT_EQ(1, node->palette().colorCount());
}

} // namespace voxelformat
