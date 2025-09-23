/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

class FBXFormatTest : public AbstractFormatTest {};

TEST_F(FBXFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "chr_knight.fbx", 17);
	const scenegraph::SceneGraphNode *modelNode = sceneGraph.firstModelNode();
	ASSERT_NE(modelNode, nullptr);
	EXPECT_EQ(modelNode->name(), "K_Foot_Right");
	EXPECT_EQ(modelNode->children().size(), 0u);
}

// TODO: VOXELFORMAT: we currently don't have fbx material write support - and ufbx can't load ascii files
TEST_F(FBXFormatTest, DISABLED_testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	testMaterial(sceneGraph, "test_material.fbx");
}

} // namespace voxelformat
