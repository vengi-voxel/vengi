/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/FBXFormat.h"

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

TEST_F(FBXFormatTest, testLoadAnimation) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "chr_knight.fbx", 17);
	// The file has one animation stack: "Take 001"
	const auto &animations = sceneGraph.animations();
	ASSERT_GE(animations.size(), 2u); // Default + Take 001
	bool foundTake = false;
	for (const auto &anim : animations) {
		if (anim == "Take 001") {
			foundTake = true;
		}
	}
	EXPECT_TRUE(foundTake) << "Expected 'Take 001' animation";

	// Check that animated group nodes have keyframes
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (node.name() == "Handle_Root") {
			ASSERT_TRUE(node.allKeyFrames().hasKey("Take 001"));
			const auto &kfs = node.keyFrames("Take 001");
			EXPECT_GT(kfs.size(), 1u) << "Handle_Root should have multiple keyframes";
			// Verify scale is imported
			const auto &s = kfs[0].transform().localScale();
			EXPECT_FLOAT_EQ(s.x, 1.0f);
			EXPECT_FLOAT_EQ(s.y, 1.0f);
			EXPECT_FLOAT_EQ(s.z, 1.0f);
			break;
		}
	}

	// Static mesh nodes should have only 1 keyframe for Take 001
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (node.name() == "K_Foot_Right" && node.allKeyFrames().hasKey("Take 001")) {
			const auto &kfs = node.keyFrames("Take 001");
			EXPECT_EQ(kfs.size(), 1u) << "Static mesh node should have 1 keyframe";
			break;
		}
	}
}

TEST_F(FBXFormatTest, testSaveSingleVoxel) {
	FBXFormat f;
	const voxel::ValidateFlags flags = (voxel::ValidateFlags::Mesh & ~(voxel::ValidateFlags::Color | voxel::ValidateFlags::Translation | voxel::ValidateFlags::Animations));
	testSaveMesh("rgb.qb", "fbx-savesingle.fbx", &f, flags);
}

// TODO: VOXELFORMAT: we currently don't have fbx material write support - and ufbx can't load ascii files
TEST_F(FBXFormatTest, DISABLED_testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	testMaterial(sceneGraph, "test_material.fbx");
}

} // namespace voxelformat
