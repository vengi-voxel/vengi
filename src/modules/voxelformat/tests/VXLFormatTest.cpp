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
	EXPECT_VEC_NEAR(glm::vec3(0.5136022f, 0.1523392f, 0.5000000f), node1.pivot(), 0.001f);
	const scenegraph::SceneGraphKeyFrame *keyframe1 = node1.keyFrame(0);
	ASSERT_NE(nullptr, keyframe1);
	const glm::mat4 expected = {{1.0f, 0.0f, 0.0f, 0.0f},
								{0.0f, 1.0f, 0.0f, 0.0f},
								{0.0f, 0.0f, 1.0f, 0.0f},
								{-14.9761734f, 0.8904828f, 7.3924913f, 1.0f}};
	EXPECT_MAT_NEAR(expected, keyframe1->transform().localMatrix(), 0.001f);

	const scenegraph::SceneGraphNode &node2 = sceneGraph.node(2);
	ASSERT_EQ(2, node2.id());
	EXPECT_EQ("LEG R REAR LOWE", node2.name());
	EXPECT_VEC_NEAR(glm::vec3(0.3476858f, 0.8018382f, 0.5000001f), node2.pivot(), 0.001f);
	const glm::mat4 expected2 = {{1.0f, 0.0f, 0.0f, 0.0f},
								 {0.0f, 1.0f, 0.0f, 0.0f},
								 {0.0f, 0.0f, 1.0f, 0.0f},
								 {-18.2648506f, 9.9278526f, 6.7404451f, 1.0f}};
	const scenegraph::SceneGraphKeyFrame *keyframe2 = node2.keyFrame(0);
	ASSERT_NE(nullptr, keyframe2);
	EXPECT_MAT_NEAR(expected2, keyframe2->transform().localMatrix(), 0.001f);

	const scenegraph::SceneGraphNode &node3 = sceneGraph.node(3);
	ASSERT_EQ(3, node3.id());
	EXPECT_EQ("LEG R REAR UPPE", node3.name());
	EXPECT_VEC_NEAR(glm::vec3(0.6807217f, 0.7823251f, 0.5000000f), node3.pivot(), 0.001f);
	const glm::mat4 expected3 = {{1.0f, 0.0f, 0.0f, 0.0f},
								 {0.0f, 1.0f, 0.0f, 0.0f},
								 {0.0f, 0.0f, 1.0f, 0.0f},
								 {-13.6222296f, 16.6269493f, 5.8894053f, 1.0f}};
	const scenegraph::SceneGraphKeyFrame *keyframe3 = node3.keyFrame(0);
	ASSERT_NE(nullptr, keyframe3);
	EXPECT_MAT_NEAR(expected3, keyframe3->transform().localMatrix(), 0.001f);

	const scenegraph::SceneGraphNode &node4 = sceneGraph.node(4);
	ASSERT_EQ(4, node4.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5063228f, 0.1407128f, 0.5), node4.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node5 = sceneGraph.node(5);
	ASSERT_EQ(5, node5.id());
	EXPECT_VEC_NEAR(glm::vec3(0.4745446f, 0.7694066f, 0.5f), node5.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node6 = sceneGraph.node(6);
	ASSERT_EQ(6, node6.id());
	EXPECT_VEC_NEAR(glm::vec3(0.3289936f, 0.8072720f, 0.5f), node6.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node7 = sceneGraph.node(7);
	ASSERT_EQ(7, node7.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5145851f, 0.1433197f, 0.5f), node7.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node8 = sceneGraph.node(8);
	ASSERT_EQ(8, node8.id());
	EXPECT_VEC_NEAR(glm::vec3(0.3238616f, 0.7899737f, 0.5f), node8.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node9 = sceneGraph.node(9);
	ASSERT_EQ(9, node9.id());
	EXPECT_VEC_NEAR(glm::vec3(0.3545019f, 0.7885991f, 0.5f), node9.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node10 = sceneGraph.node(10);
	ASSERT_EQ(10, node10.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5174056f, 0.1425452f, 0.5f), node10.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node11 = sceneGraph.node(11);
	ASSERT_EQ(11, node11.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5406196f, 0.7914940f, 0.5f), node11.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node12 = sceneGraph.node(12);
	ASSERT_EQ(12, node12.id());
	EXPECT_VEC_NEAR(glm::vec3(0.6362115f, 0.7923990f, 0.5f), node12.pivot(), 0.001f);

	const scenegraph::SceneGraphNode &node13 = sceneGraph.node(13);
	ASSERT_EQ(13, node13.id());
	EXPECT_VEC_NEAR(glm::vec3(0.5f, 0.4826846f, 0.5054069f), node13.pivot(), 0.001f);
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
