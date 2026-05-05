/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/tests/TestHelper.h"
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
	// SceneGraphModelsParent excluded: FBX import adds an intermediate root node from ufbx
	const voxel::ValidateFlags flags = (voxel::ValidateFlags::Mesh & ~(voxel::ValidateFlags::Color | voxel::ValidateFlags::Translation | voxel::ValidateFlags::Animations | voxel::ValidateFlags::SceneGraphModelsParent));
	testSaveMesh("rgb.qb", "fbx-savesingle.fbx", &f, flags);
}

TEST_F(FBXFormatTest, testAnimationRoundTrip) {
	FBXFormat f;
	const io::ArchivePtr &archive = helper_filesystemarchive();
	scenegraph::SceneGraph srcGraph;
	ASSERT_TRUE(f.load("chr_knight.fbx", archive, srcGraph, testLoadCtx));
	ASSERT_EQ(srcGraph.size(scenegraph::SceneGraphNodeType::AllModels), 17u);

	// Save to FBX
	const core::String outFile = "chr_knight-roundtrip.fbx";
	ASSERT_TRUE(f.save(srcGraph, outFile, archive, testSaveCtx));

	// Reload
	scenegraph::SceneGraph dstGraph;
	ASSERT_TRUE(f.load(outFile, archive, dstGraph, testLoadCtx));

	// Validate animations are preserved
	const auto &srcAnims = srcGraph.animations();
	const auto &dstAnims = dstGraph.animations();
	ASSERT_EQ(srcAnims.size(), dstAnims.size()) << "Animation count mismatch";
	for (const core::String &anim : srcAnims) {
		bool found = false;
		for (const core::String &dstAnim : dstAnims) {
			if (dstAnim == anim) {
				found = true;
				break;
			}
		}
		EXPECT_TRUE(found) << "Animation '" << anim.c_str() << "' not found in round-tripped file";
	}

	// Validate that animated group nodes retain their keyframes after round-trip.
	// ufbx key reduction may collapse constant channels, so we check that the majority
	// of animated nodes still have >1 keyframe.
	int srcAnimatedNodes = 0;
	int dstAnimatedNodes = 0;
	for (const auto &srcEntry : srcGraph.nodes()) {
		const scenegraph::SceneGraphNode &srcNode = srcEntry->second;
		if (!srcNode.isGroupNode()) {
			continue;
		}
		if (!srcNode.allKeyFrames().hasKey("Take 001")) {
			continue;
		}
		const auto &srcKfs = srcNode.keyFrames("Take 001");
		if (srcKfs.size() <= 1) {
			continue;
		}
		++srcAnimatedNodes;
		for (const auto &dstEntry : dstGraph.nodes()) {
			const scenegraph::SceneGraphNode &dstNode = dstEntry->second;
			if (dstNode.name() != srcNode.name()) {
				continue;
			}
			if (dstNode.allKeyFrames().hasKey("Take 001") && dstNode.keyFrames("Take 001").size() > 1) {
				++dstAnimatedNodes;
			}
			break;
		}
	}
	EXPECT_GT(srcAnimatedNodes, 0) << "Source should have animated nodes";
	EXPECT_GT(dstAnimatedNodes, 0) << "Destination should have animated nodes";
	// Allow some loss due to key reduction, but most nodes should retain animation
	EXPECT_GE(dstAnimatedNodes, srcAnimatedNodes / 2)
		<< "Too many animated nodes lost their keyframes";
}

// TODO: VOXELFORMAT: we currently don't have fbx material write support - and ufbx can't load ascii files
TEST_F(FBXFormatTest, DISABLED_testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	testMaterial(sceneGraph, "test_material.fbx");
}

} // namespace voxelformat
