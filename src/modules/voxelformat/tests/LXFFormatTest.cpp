/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "io/CachingArchive.h"
#include "io/MemoryArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "util/VarUtil.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelformat/private/mesh/lego/LegoUtil.h"

namespace voxelformat {

class LXFFormatTest : public AbstractFormatTest {
protected:
	// Pin LDraw lookup to bundled data/tests/parts so CI without /usr/share/ldraw
	// (Windows/macOS) and Linux with an official library resolve the same 3001.dat.
	scenegraph::SceneGraph loadLxfScene(const char *filename, const char *scaleValue, size_t expectedVolumes = 1) {
		util::ScopedVarChange scaleVar(cfg::VoxformatScale, scaleValue);
		util::ScopedVarChange ldrawDir(cfg::VoxformatLDrawDir, "");
		scenegraph::SceneGraph sceneGraph;
		testLoad(sceneGraph, filename, expectedVolumes, true);
		return sceneGraph;
	}
};

TEST_F(LXFFormatTest, testLoadLXFML) {
	// 0.1 so the bundled 2x4 stub (80x24x40 LDU) voxelizes to a non-empty volume.
	// Camera translation is LDD (40,40,40) * 25 LDU * scale.
	scenegraph::SceneGraph sceneGraph = loadLxfScene("lxf-simple.lxfml", "0.1");
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
	EXPECT_NEAR(camera->farPlane(), 6.9282035827636719f, 0.00001f);
	EXPECT_VEC_NEAR(glm::vec3(100.0f, -100.0f, -100.0f), cameraTransform.worldTranslation(), 0.001f);
	EXPECT_VEC_NEAR(glm::vec3(1.0f), cameraTransform.worldScale(), 0.001f);
}

TEST_F(LXFFormatTest, testRegisterLdrawSearchPathsFallback) {
	util::ScopedVarChange ldrawDir(cfg::VoxformatLDrawDir, "/nonexistent/ldraw-library");
	io::MemoryArchivePtr mem = io::openMemoryArchive();
	const uint8_t buf[] = {'0', ' ', 'B', 'r', 'i', 'c', 'k'};
	mem->add("parts/3001.dat", buf, sizeof(buf));
	io::CachingArchive cache(mem);
	legoutil::registerLdrawSearchPaths(cache);
	EXPECT_TRUE(cache.exists("3001.dat"));
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
	scenegraph::SceneGraph sceneGraph = loadLxfScene("lxf-two-parts.lxfml", "0.01", 2);
	if (IsSkipped()) {
		return;
	}
	ASSERT_EQ(sceneGraph.root().children().size(), 1u);
	const scenegraph::SceneGraphNode *groupNode = sceneGraph.findNodeByUUID(sceneGraph.root().children()[0]);
	ASSERT_NE(nullptr, groupNode);
	EXPECT_TRUE(groupNode->isGroupNode());
	EXPECT_EQ(groupNode->name(), "lxf-two-parts");
	ASSERT_EQ(groupNode->children().size(), 2u);

	const scenegraph::SceneGraphNode *firstPart = sceneGraph.findNodeByUUID(groupNode->children()[0]);
	const scenegraph::SceneGraphNode *secondPart = sceneGraph.findNodeByUUID(groupNode->children()[1]);
	ASSERT_NE(nullptr, firstPart);
	ASSERT_NE(nullptr, secondPart);
	EXPECT_TRUE(firstPart->isModelNode());
	EXPECT_TRUE(secondPart->isModelNode());
	EXPECT_EQ(firstPart->parentUUID(), groupNode->uuid());
	EXPECT_EQ(secondPart->parentUUID(), groupNode->uuid());
	EXPECT_EQ(firstPart->region().getLowerCorner(), glm::ivec3(0));
	EXPECT_EQ(secondPart->region().getLowerCorner(), glm::ivec3(0));

	sceneGraph.updateTransforms();
	const scenegraph::FrameTransform firstTransform = sceneGraph.transformForFrame(*firstPart, 0);
	const scenegraph::FrameTransform secondTransform = sceneGraph.transformForFrame(*secondPart, 0);
	EXPECT_NEAR(secondTransform.worldTranslation().x - firstTransform.worldTranslation().x, 10.0f, 0.001f);
	EXPECT_NEAR(secondTransform.worldTranslation().y - firstTransform.worldTranslation().y, 0.0f, 0.001f);
	EXPECT_NEAR(secondTransform.worldTranslation().z - firstTransform.worldTranslation().z, 0.0f, 0.001f);
}

TEST_F(LXFFormatTest, testLoadLXFMLRotatedPart) {
	scenegraph::SceneGraph sceneGraph = loadLxfScene("lxf-rot-y90.lxfml", "0.1", 2);
	if (IsSkipped()) {
		return;
	}
	ASSERT_EQ(sceneGraph.root().children().size(), 1u);
	const scenegraph::SceneGraphNode *groupNode = sceneGraph.findNodeByUUID(sceneGraph.root().children()[0]);
	ASSERT_NE(nullptr, groupNode);
	ASSERT_EQ(groupNode->children().size(), 2u);

	const scenegraph::SceneGraphNode *rotated = sceneGraph.findNodeByUUID(groupNode->children()[0]);
	const scenegraph::SceneGraphNode *reference = sceneGraph.findNodeByUUID(groupNode->children()[1]);
	ASSERT_NE(nullptr, rotated);
	ASSERT_NE(nullptr, reference);
	const glm::ivec3 rotatedSize = rotated->region().getDimensionsInCells();
	const glm::ivec3 referenceSize = reference->region().getDimensionsInCells();
	// Bundled 3001 is a 2x4 (longer along X at identity); 90 degree Y rotation swaps X/Z (vengi Y-up)
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
	for (const core::UUID &childUUID : sceneGraph.root().children()) {
		const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
		if (child != nullptr && child->isGroupNode()) {
			groupNode = child;
			break;
		}
	}
	ASSERT_NE(groupNode, nullptr);
	EXPECT_EQ(groupNode->name(), "Light1");
	EXPECT_EQ(groupNode->children().size(), 3u);
	int voxelCount = 0;
	for (const core::UUID &childUUID : groupNode->children()) {
		const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
		ASSERT_NE(nullptr, child);
		ASSERT_TRUE(child->isModelNode());
		const voxel::RawVolume *volume = child->volume();
		ASSERT_NE(volume, nullptr);
		voxelCount += voxelutil::countVoxels(*volume);
	}
	EXPECT_GT(voxelCount, 0);
}

} // namespace voxelformat
