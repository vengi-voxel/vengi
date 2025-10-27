/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "glm/fwd.hpp"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxelformat {

class BlockbenchFormatTest : public AbstractFormatTest {};

// the model comes from https://github.com/SL0ANE/Loy-s-Goodies/tree/main/models and was licensed under CC0
// version 4.5 - includes animations and a full scene with a hierarchy of nodes
TEST_F(BlockbenchFormatTest, testLoad_4_5) {
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
	const glm::vec3 expectedFirstPivot(-4.0f, 0.5f, 0.5f);
	EXPECT_VEC_NEAR(firstPivot, expectedFirstPivot, 0.0001f);
	// const glm::vec3 expectedFirstWorldPivot(8.0f, 21.9f, 0.0f);
	// const glm::vec3 firstWorldPivot = firstCube.worldPivot();
	// EXPECT_VEC_NEAR(firstWorldPivot, expectedFirstWorldPivot, 0.0001f);
	// TODO: VOXELFORMAT: this is without pivot
	// EXPECT_VEC_NEAR(firstCube.transform().worldTranslation(), glm::vec3(12.0f, 20.9f, -1.0f), 0.0001f);
	// TODO: VOXELFORMAT: check voxels and colors

	ASSERT_EQ(secondCube.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::Region &secondRegion = secondCube.volume()->region();
	const glm::ivec3 &secondDim = secondRegion.getDimensionsInVoxels();
	EXPECT_EQ(secondDim.x, 1);
	EXPECT_EQ(secondDim.y, 2);
	EXPECT_EQ(secondDim.z, 2);
	ASSERT_EQ(fingers_right.type(), scenegraph::SceneGraphNodeType::Group);

	ASSERT_EQ(sceneGraph.animations().size(), 5u);
	EXPECT_TRUE(sceneGraph.hasAnimation("handsdown"));
	EXPECT_TRUE(sceneGraph.hasAnimation("sit"));
	EXPECT_TRUE(sceneGraph.hasAnimation("fist"));
	EXPECT_TRUE(sceneGraph.hasAnimation("walk"));
	EXPECT_TRUE(sceneGraph.hasAnimation("run"));
}

// this model is based on a model from https://github.com/SL0ANE/Loy-s-Goodies/tree/main/models - but only one cube was
// extracted to simplify the scene this was done in the web based version of blockbench - https://web.blockbench.net/ -
// on saving the file the version was updated to 4.10
TEST_F(BlockbenchFormatTest, testLoad_4_10) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "female_template_head_4_10.bbmodel", 1);
	scenegraph::SceneGraphNode *cube = sceneGraph.findNodeByName("cube");
	ASSERT_NE(cube, nullptr);
	ASSERT_EQ(cube->children().size(), 0u);
	voxel::RawVolume *volume = cube->volume();
	ASSERT_NE(volume, nullptr);
	const voxel::Region &region = volume->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	EXPECT_EQ(dim.x, 8);
	EXPECT_EQ(dim.y, 8);
	EXPECT_EQ(dim.z, 8);
	const voxel::Voxel v1 = volume->voxel(region.getLowerCenter());
	EXPECT_TRUE(voxel::isBlocked(v1.getMaterial()));
	// TODO: VOXELFORMAT: compare colors - needed because we overwrite the color while loading all faces for a cube
}

// this model was created in the blockbench web edition and includes all mesh types
TEST_F(BlockbenchFormatTest, testLoadMeshTypes) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "blockbench_meshtypes.bbmodel", 10);
}

} // namespace voxelformat
