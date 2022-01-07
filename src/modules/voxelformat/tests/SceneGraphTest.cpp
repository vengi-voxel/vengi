/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxel {

class SceneGraphTest: public app::AbstractTest {
};

TEST_F(SceneGraphTest, testSize) {
	ScopedSceneGraph sceneGraph;
	EXPECT_EQ(1u, sceneGraph.size(SceneGraphNodeType::Root)) << "Each scene graph should contain a root node by default";
	EXPECT_TRUE(sceneGraph.empty()) << "There are no model nodes yet - thus empty should return true";
	{
		SceneGraphNode node;
		node.setName("node1");
		sceneGraph.emplace_back(core::move(node));
	}
	{
		SceneGraphNode node;
		node.setName("node2");
		sceneGraph.emplace_back(core::move(node));
	}
	EXPECT_EQ(2u, sceneGraph.size(SceneGraphNodeType::Model)) << "The scene graph should have two models";
	EXPECT_EQ(2u, sceneGraph.size()) << "The scene graph should have two models";

	EXPECT_EQ(2u, sceneGraph.root().children().size()) << "The root node should have two (model) children attached";
}

}
