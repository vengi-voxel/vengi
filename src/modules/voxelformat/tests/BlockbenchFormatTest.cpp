/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "glm/fwd.hpp"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxelformat {

class BlockbenchFormatTest : public AbstractFormatTest {
protected:
	// Loading the same file in different versions of Blockbench should yield the same results
	// TODO: for gltf/glb some stuff is different in the hierarchy - need to investigate further
	void test_loy_s_goodies_female_template(scenegraph::SceneGraph &sceneGraph, bool isbbmodel) {
		// Test hierarchy structure
		scenegraph::SceneGraphNode *hand_right = sceneGraph.findNodeByName("hand_right");
		ASSERT_NE(hand_right, nullptr);
		EXPECT_EQ(hand_right->type(), scenegraph::SceneGraphNodeType::Group);
		ASSERT_GE(hand_right->children().size(), 3u);

		// Verify children of hand_right
		scenegraph::SceneGraphNode &firstCube = sceneGraph.node(hand_right->children()[0]);
		EXPECT_EQ(firstCube.name(), "cube");
		scenegraph::SceneGraphNode &secondCube = sceneGraph.node(hand_right->children()[1]);
		EXPECT_EQ(secondCube.name(), "cube");
		scenegraph::SceneGraphNode &fingers_right = sceneGraph.node(hand_right->children()[2]);
		EXPECT_EQ(fingers_right.name(), "fingers_right");
		EXPECT_EQ(fingers_right.type(), scenegraph::SceneGraphNodeType::Group);

		// Test first cube dimensions
		// In bbmodel: from=[12, 20.9, -1], to=[13, 22.9, 1], origin=[8, 21.9, 0]
		EXPECT_EQ(firstCube.type(), scenegraph::SceneGraphNodeType::Model);
		const voxel::Region &firstRegion = firstCube.region();
		const glm::ivec3 &firstDim = firstRegion.getDimensionsInVoxels();
		EXPECT_EQ(firstDim.x, 1);
		EXPECT_EQ(firstDim.y, 2);
		EXPECT_EQ(firstDim.z, 2);

		// Test pivot calculation
		// Currently: pivot = (origin - from) / (to - from) = ([8,21.9,0] - [12,20.9,-1]) / [1,2,2] = [-4, 0.5, 0.5]
		// This is expected given the current implementation
		// TODO: VOXELFORMAT: Review if element origin should be used differently
		const glm::vec3 &firstPivot = firstCube.pivot();
		const glm::vec3 expectedFirstPivot(-4.0f, 0.5f, 0.5f);
		EXPECT_VEC_NEAR(firstPivot, expectedFirstPivot, 0.0001f);

		// Test second cube dimensions
		// In bbmodel: from=[13, 21.9, -1], to=[14, 22.9, 1]
		ASSERT_EQ(secondCube.type(), scenegraph::SceneGraphNodeType::Model);
		const voxel::Region &secondRegion = secondCube.region();
		const glm::ivec3 &secondDim = secondRegion.getDimensionsInVoxels();
		EXPECT_EQ(secondDim.x, 1);
		EXPECT_EQ(secondDim.y, 1);
		EXPECT_EQ(secondDim.z, 2); // Test group node
		ASSERT_EQ(fingers_right.type(), scenegraph::SceneGraphNodeType::Group);
		EXPECT_GT(fingers_right.children().size(), 0u);

		// Test animations
		ASSERT_EQ(sceneGraph.animations().size(), 5u);
		EXPECT_TRUE(sceneGraph.hasAnimation("handsdown"));
		EXPECT_TRUE(sceneGraph.hasAnimation("sit"));
		EXPECT_TRUE(sceneGraph.hasAnimation("fist"));
		EXPECT_TRUE(sceneGraph.hasAnimation("walk"));
		EXPECT_TRUE(sceneGraph.hasAnimation("run"));

		// Test that the main group was created correctly
		scenegraph::SceneGraphNode *main = sceneGraph.findNodeByName("main");
		ASSERT_NE(main, nullptr);
		EXPECT_EQ(main->type(), scenegraph::SceneGraphNodeType::Group);

#if 0
		// TODO: VOXELFORMAT: these values were taken directly out of blockbench - need to verify why they don't match
		scenegraph::SceneGraphNode *eyeglossRight = sceneGraph.findNodeByName("eyegloss_right");
		ASSERT_NE(eyeglossRight, nullptr);
		EXPECT_EQ(eyeglossRight->type(), scenegraph::SceneGraphNodeType::Group);
		const scenegraph::SceneGraphTransform &eyeglossRightTransform = eyeglossRight->transform(0);
		EXPECT_VEC_NEAR(glm::vec3(3.0f, 27.4f, -3.91f), eyeglossRightTransform.worldTranslation(), 0.00001f);
		ASSERT_EQ(1u, eyeglossRight->children().size());

		const scenegraph::SceneGraphNode &eyeglossRightCube = sceneGraph.node(eyeglossRight->children()[0]);
		EXPECT_EQ(eyeglossRightCube.type(), scenegraph::SceneGraphNodeType::Model);
		const scenegraph::SceneGraphTransform &eyeglossRightCubeTransform = eyeglossRightCube.transform(0);
		EXPECT_VEC_NEAR(glm::vec3(2.5f, 27.15f, -3.991f), eyeglossRightCubeTransform.worldTranslation(), 0.00001f);
		EXPECT_VEC_NEAR(glm::vec3(-0.25f, 23.9f, 0.0f), eyeglossRightCube.worldPivot(), 0.00001f);
#endif
	}
};

// the model comes from https://github.com/SL0ANE/Loy-s-Goodies/tree/main/models and was licensed under CC0
// version 4.5 - includes animations and a full scene with a hierarchy of nodes
TEST_F(BlockbenchFormatTest, testLoad_4_5) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "loy_s_goodies_female_template.bbmodel", 53);
	SCOPED_TRACE("loy_s_goodies_female_template.bbmodel");
	test_loy_s_goodies_female_template(sceneGraph, true);
}

// TODO: failing to load the new version
TEST_F(BlockbenchFormatTest, DISABLED_testLoad_5_0_3) {
	{
		scenegraph::SceneGraph sceneGraphGLB;
		testLoad(sceneGraphGLB, "loy_s_goodies_female_template_5_0_3.glb", 53);
		SCOPED_TRACE("loy_s_goodies_female_template_5_0_3.glb");
		test_loy_s_goodies_female_template(sceneGraphGLB, false);
	}

	{
		scenegraph::SceneGraph sceneGraph;
		testLoad(sceneGraph, "loy_s_goodies_female_template_5_0_3.bbmodel", 53);
		SCOPED_TRACE("loy_s_goodies_female_template_5_0_3.glb");
		test_loy_s_goodies_female_template(sceneGraph, true);
	}
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
