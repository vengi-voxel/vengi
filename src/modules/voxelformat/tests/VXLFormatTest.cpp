/**
 * @file
 */

#include "voxelformat/private/commandconquer/VXLFormat.h"
#include "AbstractFormatTest.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxelformat {

class VXLFormatTest : public AbstractFormatTest {};

TEST_F(VXLFormatTest, testLoad) {
	testLoad("cc.vxl");
}

TEST_F(VXLFormatTest, testLoadRGB) {
	testRGB("rgb.vxl");
}

TEST_F(VXLFormatTest, testSaveAndLoadSceneGraph) {
	VXLFormat f;
	testLoadSaveAndLoadSceneGraph("cc.vxl", f, "cc-save.vxl", f);
}

TEST_F(VXLFormatTest, testSaveAndLoadSceneGraphWithAnimations) {
	VXLFormat f;
	testLoadSaveAndLoadSceneGraph("hmec.vxl", f, "hmec-save.vxl", f);
}

TEST_F(VXLFormatTest, testHMECVXLAndHVA) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "hmec.vxl", 13);

	// TODO: VOXELFORMAT: https://github.com/vengi-voxel/vengi/issues/636
	// I am not sure if these expected values are correct - they were taken from
	// the original VXL + HVA files loaded into vengi - but at least the hmec model
	// looks correct when rendered.
	const scenegraph::SceneGraphNode &node1 = sceneGraph.node(1);
	ASSERT_EQ(1, node1.id());
	EXPECT_EQ("FOOT R REAR", node1.name());
	EXPECT_VEC_NEAR(glm::vec3(0.4991801f, 0.1488506f, 0.4687738f), node1.pivot(), 0.001f);
	const scenegraph::SceneGraphKeyFrame *keyframe1 = node1.keyFrame(0);
	ASSERT_NE(nullptr, keyframe1);
	const glm::mat4 expected = {{0.971920, -0.000000, 0.000000, 0.000000},
								{0.000000, 0.977100, -0.000000, 0.000000},
								{-0.000000, 0.000000, 0.937548, 0.000000},
								{-14.555638, 0.870091, 6.930813, 1.000000}};
	EXPECT_MAT_NEAR(expected, keyframe1->transform().localMatrix(), 0.001f);

	const scenegraph::SceneGraphNode &node2 = sceneGraph.node(2);
	ASSERT_EQ(2, node2.id());
	EXPECT_EQ("LEG R REAR LOWE", node2.name());
	EXPECT_VEC_NEAR(glm::vec3(0.3254017f, 0.7413985f, 0.4742618f), node2.pivot(), 0.001f);
	const glm::mat4 expected2 = {{0.935907, -0.000000, 0.000000, 0.000000},
									{0.000000, 0.924624, -0.000000, 0.000000},
									{-0.000000, 0.000000, 0.948524, 0.000000},
									{-17.094210, 9.179527, 6.393470, 1.000000}};
	const scenegraph::SceneGraphKeyFrame *keyframe2 = node2.keyFrame(0);
	ASSERT_NE(nullptr, keyframe2);
	EXPECT_MAT_NEAR(expected2, keyframe2->transform().localMatrix(), 0.001f);

	const scenegraph::SceneGraphNode &node3 = sceneGraph.node(3);
	ASSERT_EQ(3, node3.id());
	EXPECT_EQ("LEG R REAR UPPE", node3.name());
	EXPECT_VEC_NEAR(glm::vec3(0.6801815f, 0.7377839f, 0.4384358f), node3.pivot(), 0.001f);
	const glm::mat4 expected3 = {{0.999207, -0.000000, 0.000000, 0.000000},
									{0.000000, 0.943066, -0.000000, 0.000000},
									{-0.000000, 0.000000, 0.876872, 0.000000},
									{-13.611420, 15.680304, 5.164252, 1.000000}};
	const scenegraph::SceneGraphKeyFrame *keyframe3 = node3.keyFrame(0);
	ASSERT_NE(nullptr, keyframe3);
	EXPECT_MAT_NEAR(expected3, keyframe3->transform().localMatrix(), 0.001f);

	const scenegraph::SceneGraphNode &node4 = sceneGraph.node(4);
	ASSERT_EQ(4, node4.id());
	EXPECT_VEC_NEAR(glm::vec3(0.4933756f, 0.1215701f, 0.4703021f), node4.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node5 = sceneGraph.node(5);
	ASSERT_EQ(5, node5.id());
	EXPECT_VEC_NEAR(glm::vec3(0.4692247f, 0.7465140f, 0.4742619f), node5.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node6 = sceneGraph.node(6);
	ASSERT_EQ(6, node6.id());
	EXPECT_VEC_NEAR(glm::vec3(0.2996698f, 0.7520319f, 0.4384358f), node6.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node7 = sceneGraph.node(7);
	ASSERT_EQ(7, node7.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5001363f, 0.1396166f, 0.4687738f), node7.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node8 = sceneGraph.node(8);
	ASSERT_EQ(8, node8.id());
	EXPECT_VEC_NEAR(glm::vec3(0.3062816f, 0.7695775f, 0.4742619f), node8.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node9 = sceneGraph.node(9);
	ASSERT_EQ(9, node9.id());
	EXPECT_VEC_NEAR(glm::vec3(0.3225202f, 0.7753450f, 0.4384359f), node9.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node10 = sceneGraph.node(10);
	ASSERT_EQ(10, node10.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5028766f, 0.1231306f, 0.4687738f), node10.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node11 = sceneGraph.node(11);
	ASSERT_EQ(11, node11.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5147860f, 0.7881539f, 0.4742618f), node11.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node12 = sceneGraph.node(12);
	ASSERT_EQ(12, node12.id());
	EXPECT_VEC_NEAR(glm::vec3(0.6035370f, 0.7666695f, 0.4384359f), node12.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node13 = sceneGraph.node(13);
	ASSERT_EQ(13, node13.id());
	EXPECT_VEC_NEAR(glm::vec3(0.4914285f, 0.4807197f, 0.494368f), node13.pivot(), 0.001f);
}

// https://github.com/vengi-voxel/vengi/issues/636
TEST_F(VXLFormatTest, testLegsVXLAndHVAIssue636) {
	scenegraph::SceneGraph sceneGraph;
	int expectedModels = 18;
	testLoad(sceneGraph, "bug636/legs.vxl", expectedModels);

	// TODO: VOXELFORMAT: implement me
}

TEST_F(VXLFormatTest, testSaveSmallVoxel) {
	VXLFormat f;
	testSaveLoadVoxel("cc-smallvolumesavetest.vxl", &f, 0, 1,
					  voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

} // namespace voxelformat
