/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "glm/fwd.hpp"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

class BlockbenchFormatTest : public AbstractFormatTest {};

TEST_F(BlockbenchFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "loy_s_goodies_female_template.bbmodel", 53);
	scenegraph::SceneGraphNode *hand_right = sceneGraph.findNodeByName("hand_right");
	ASSERT_NE(hand_right, nullptr);
	ASSERT_EQ(hand_right->children().size(), 3u);
	scenegraph::SceneGraphNode &firstCube = sceneGraph.node(hand_right->children()[0]);
	ASSERT_EQ(firstCube.name(), "cube");
	scenegraph::SceneGraphNode &secondCube = sceneGraph.node(hand_right->children()[0]);
	ASSERT_EQ(secondCube.name(), "cube");
	scenegraph::SceneGraphNode &fingers_right = sceneGraph.node(hand_right->children()[2]);
	ASSERT_EQ(fingers_right.name(), "fingers_right");

	ASSERT_EQ(firstCube.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::Region &firstRegion = firstCube.volume()->region();
	const glm::ivec3 &firstDim = firstRegion.getDimensionsInVoxels();
	EXPECT_EQ(firstDim.x, 1);
	EXPECT_EQ(firstDim.y, 2);
	EXPECT_EQ(firstDim.z, 2);
	const glm::vec3 &firstPivot = firstCube.pivot();
	const glm::vec3	expectedFirstPivot(-4.0f, 0.5f, 0.5f);
	EXPECT_VEC_NEAR(firstPivot, expectedFirstPivot, 0.0001f);
	// const glm::vec3 expectedFirstWorldPivot(8.0f, 21.9f, 0.0f);
	// const glm::vec3 firstWorldPivot = firstCube.worldPivot();
	// EXPECT_VEC_NEAR(firstWorldPivot, expectedFirstWorldPivot, 0.0001f);
	EXPECT_VEC_NEAR(firstCube.transform().worldTranslation(), glm::vec3(12.0f, 20.9f, -1.0f), 0.0001f);
	// TODO: check voxels and colors

	ASSERT_EQ(secondCube.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::Region &secondRegion = secondCube.volume()->region();
	const glm::ivec3 &secondDim = secondRegion.getDimensionsInVoxels();
	EXPECT_EQ(secondDim.x, 1);
	EXPECT_EQ(secondDim.y, 2);
	EXPECT_EQ(secondDim.z, 2);
	ASSERT_EQ(fingers_right.type(), scenegraph::SceneGraphNodeType::Group);
}

} // namespace voxelformat
