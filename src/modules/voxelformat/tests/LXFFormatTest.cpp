/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "util/VarUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class LXFFormatTest : public AbstractFormatTest {
protected:
	scenegraph::SceneGraph loadLxfScene(const char *filename, const char *scaleValue) {
		util::ScopedVarChange scaleVar(cfg::VoxformatScale, scaleValue);
		scenegraph::SceneGraph sceneGraph;
		testLoad(sceneGraph, filename, 1, true);
		return sceneGraph;
	}
};

TEST_F(LXFFormatTest, testLoadLXFML) {
	scenegraph::SceneGraph sceneGraph = loadLxfScene("lxf-simple.lxfml", "0.01");
	if (IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	EXPECT_EQ(root.property("lxfml_name"), "lxf-simple");
	EXPECT_EQ(root.property("lxfml_versionMajor"), "5");
	EXPECT_EQ(root.property("lxfml_versionMinor"), "0");
	EXPECT_EQ(root.property("ldd_application_name"), "LEGO Digital Designer");
	EXPECT_EQ(root.property("ldd_brand_name"), "LDD");
	EXPECT_EQ(root.property("ldd_brickset_version"), "1564.2");
	EXPECT_EQ(root.property("lxfml_camera_ref"), "0");

	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(node->name(), "lxf-simple");
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(volume, nullptr);
	EXPECT_GT(voxelutil::countVoxels(*volume), 0);

	sceneGraph.updateTransforms();
	const scenegraph::SceneGraphNodeCamera *camera = sceneGraph.activeCameraNode();
	ASSERT_NE(camera, nullptr);
	const scenegraph::FrameTransform cameraTransform = sceneGraph.transformForFrame(*camera, 0);
	EXPECT_EQ(camera->fieldOfView(), 80);
	EXPECT_NEAR(camera->farPlane(), 0.69282035827636719f, 0.00001f);
	EXPECT_VEC_NEAR(glm::vec3(10.0f, -10.0f, -10.0f), cameraTransform.worldTranslation(), 0.001f);
	EXPECT_VEC_NEAR(glm::vec3(1.0f), cameraTransform.worldScale(), 0.001f);
}

TEST_F(LXFFormatTest, testLoadAmsterdamCanalStreet) {
	util::ScopedVarChange scaleVar(cfg::VoxformatScale, "0.01");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test.lxf", 1, true);
	if (IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(volume, nullptr);
	EXPECT_GT(voxelutil::countVoxels(*volume), 100);
	EXPECT_FALSE(sceneGraph.root().property("lxfml_name").empty());
}

TEST_F(LXFFormatTest, testLoadLXFMLPartHierarchy) {
	util::ScopedVarChange scaleVar(cfg::VoxformatScale, "0.01");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "lxf-two-parts.lxfml", 2, true);
	if (IsSkipped()) {
		return;
	}
	ASSERT_EQ(sceneGraph.root().children().size(), 1u);
	const scenegraph::SceneGraphNode &groupNode = sceneGraph.node(sceneGraph.root().children()[0]);
	EXPECT_TRUE(groupNode.isGroupNode());
	EXPECT_EQ(groupNode.name(), "lxf-two-parts");
	ASSERT_EQ(groupNode.children().size(), 2u);

	const scenegraph::SceneGraphNode &firstPart = sceneGraph.node(groupNode.children()[0]);
	const scenegraph::SceneGraphNode &secondPart = sceneGraph.node(groupNode.children()[1]);
	EXPECT_TRUE(firstPart.isModelNode());
	EXPECT_TRUE(secondPart.isModelNode());
	EXPECT_EQ(firstPart.parent(), groupNode.id());
	EXPECT_EQ(secondPart.parent(), groupNode.id());
	EXPECT_EQ(firstPart.region().getLowerCorner(), glm::ivec3(0));
	EXPECT_EQ(secondPart.region().getLowerCorner(), glm::ivec3(0));

	sceneGraph.updateTransforms();
	const scenegraph::FrameTransform firstTransform = sceneGraph.transformForFrame(firstPart, 0);
	const scenegraph::FrameTransform secondTransform = sceneGraph.transformForFrame(secondPart, 0);
	EXPECT_NEAR(secondTransform.worldTranslation().x - firstTransform.worldTranslation().x, 10.0f, 0.001f);
	EXPECT_NEAR(secondTransform.worldTranslation().y - firstTransform.worldTranslation().y, 0.0f, 0.001f);
	EXPECT_NEAR(secondTransform.worldTranslation().z - firstTransform.worldTranslation().z, 0.0f, 0.001f);
}

TEST_F(LXFFormatTest, testLoadLXFMLRotatedPart) {
	util::ScopedVarChange scaleVar(cfg::VoxformatScale, "0.1");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "lxf-rot-y90.lxfml", 2, true);
	if (IsSkipped()) {
		return;
	}
	ASSERT_EQ(sceneGraph.root().children().size(), 1u);
	const scenegraph::SceneGraphNode &groupNode = sceneGraph.node(sceneGraph.root().children()[0]);
	ASSERT_EQ(groupNode.children().size(), 2u);

	const scenegraph::SceneGraphNode &rotated = sceneGraph.node(groupNode.children()[0]);
	const scenegraph::SceneGraphNode &reference = sceneGraph.node(groupNode.children()[1]);
	const glm::ivec3 rotatedSize = rotated.region().getDimensionsInCells();
	const glm::ivec3 referenceSize = reference.region().getDimensionsInCells();
	// 3001 is longer along Z at identity; 90 degree Y rotation swaps X/Z extents (vengi Y-up)
	EXPECT_GT(rotatedSize.z, rotatedSize.x);
	EXPECT_GT(referenceSize.x, referenceSize.z);
	EXPECT_GT(rotatedSize.z, referenceSize.z);
	EXPECT_LT(rotatedSize.x, referenceSize.x);
}

TEST_F(LXFFormatTest, testLoadLXFMLLight1Fixture) {
	util::ScopedVarChange scaleVar(cfg::VoxformatScale, "0.01");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "lxf-light1.lxfml", 3, true);
	if (IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *groupNode = nullptr;
	for (int childId : sceneGraph.root().children()) {
		const scenegraph::SceneGraphNode &child = sceneGraph.node(childId);
		if (child.isGroupNode()) {
			groupNode = &child;
			break;
		}
	}
	ASSERT_NE(groupNode, nullptr);
	EXPECT_EQ(groupNode->name(), "Light1");
	EXPECT_EQ(groupNode->children().size(), 3u);
	int voxelCount = 0;
	for (int childId : groupNode->children()) {
		const scenegraph::SceneGraphNode &child = sceneGraph.node(childId);
		ASSERT_TRUE(child.isModelNode());
		const voxel::RawVolume *volume = child.volume();
		ASSERT_NE(volume, nullptr);
		voxelCount += voxelutil::countVoxels(*volume);
	}
	EXPECT_GT(voxelCount, 0);
}

} // namespace voxelformat
