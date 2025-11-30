/**
 * @file
 */

#include "scenegraph/SceneGraph.h"
#include "app/tests/AbstractTest.h"
#include "color/Color.h"
#include "core/ScopedPtr.h"
#include "math/OBB.h"
#include "palette/FormatConfig.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/tests/TestHelper.h"
#include "palette/tests/TestHelper.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/tests/VoxelPrinter.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/gtc/quaternion.hpp>

namespace scenegraph {

class SceneGraphTest : public app::AbstractTest {
public:
	void SetUp() override {
		app::AbstractTest::SetUp();
		palette::FormatConfig::init();
	}
};

TEST_F(SceneGraphTest, testSize) {
	SceneGraph sceneGraph;
	EXPECT_EQ(1u, sceneGraph.size(SceneGraphNodeType::Root))
		<< "Each scene graph should contain a root node by default";
	EXPECT_TRUE(sceneGraph.empty()) << "There are no model nodes yet - thus empty should return true";
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("node1");
		sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("node2");
		sceneGraph.emplace(core::move(node));
	}
	EXPECT_EQ(2u, sceneGraph.size(SceneGraphNodeType::Group)) << "The scene graph should have two groups";
	EXPECT_EQ(0u, sceneGraph.size()) << "The scene graph should have no models";

	EXPECT_EQ(2u, sceneGraph.root().children().size()) << "The root node should have two (group) children attached";
}

TEST_F(SceneGraphTest, testHasNode) {
	SceneGraph sceneGraph;
	EXPECT_TRUE(sceneGraph.hasNode(0));
	EXPECT_FALSE(sceneGraph.hasNode(1));
	SceneGraphNode node(SceneGraphNodeType::Group);
	node.setName("node");
	EXPECT_EQ(1, sceneGraph.emplace(core::move(node)));
	EXPECT_TRUE(sceneGraph.hasNode(0));
	EXPECT_TRUE(sceneGraph.hasNode(1));
	EXPECT_FALSE(sceneGraph.hasNode(2));
}

TEST_F(SceneGraphTest, testNodeRoot) {
	SceneGraph sceneGraph;
	const SceneGraphNode &root = sceneGraph.node(0);
	EXPECT_EQ(0, root.id());
	EXPECT_EQ(SceneGraphNodeType::Root, root.type());
}

TEST_F(SceneGraphTest, testNode) {
	SceneGraph sceneGraph;
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("node");
		sceneGraph.emplace(core::move(node));
	}
	const SceneGraphNode &groupNode = sceneGraph.node(1);
	EXPECT_EQ(SceneGraphNodeType::Group, groupNode.type());
	EXPECT_EQ(1, groupNode.id());
	EXPECT_EQ("node", groupNode.name());
}

TEST_F(SceneGraphTest, testPaletteMergeSingleNode) {
	SceneGraph sceneGraph;
	palette::Palette pal;
	pal.nippon();
	voxel::RawVolume v(voxel::Region(0, 1));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("model1");
		node.setPalette(pal);
		EXPECT_GT(sceneGraph.emplace(core::move(node), 0), 0);
	}
	const palette::Palette &palette = sceneGraph.mergePalettes(true);
	ASSERT_EQ(palette.colorCount(), pal.colorCount()) << palette << pal;
	for (int i = 0; i < pal.colorCount(); ++i) {
		const color::RGBA c1 = palette.color(i);
		const color::RGBA c2 = pal.color(i);
		ASSERT_EQ(c1, c2) << "Color at index " << i << " differs: " << color::Color::print(c1, true) << " != " << color::Color::print(c2, true);
	}
	ASSERT_EQ(palette.hash(), pal.hash()) << palette << pal;
}

TEST_F(SceneGraphTest, testPaletteMergeSkipFirst) {
	SceneGraph sceneGraph;
	palette::Palette pal;
	pal.nippon();
	voxel::RawVolume v(voxel::Region(0, 1));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("model1");
		node.setPalette(pal);
		EXPECT_GT(sceneGraph.emplace(core::move(node), 0), 0);
	}
	const palette::Palette &palette = sceneGraph.mergePalettes(true, 0);
	ASSERT_EQ(palette.colorCount(), pal.colorCount()) << palette << pal;
}

TEST_F(SceneGraphTest, testPaletteMergeSkipLast) {
	SceneGraph sceneGraph;
	palette::Palette pal;
	pal.nippon();
	voxel::RawVolume v(voxel::Region(0, 1));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("model1");
		node.setPalette(pal);
		EXPECT_GT(sceneGraph.emplace(core::move(node), 0), 0);
	}
	const palette::Palette &palette = sceneGraph.mergePalettes(true, 255);
	ASSERT_EQ(palette.colorCount(), pal.colorCount()) << palette << pal;
}

TEST_F(SceneGraphTest, testPaletteMergeSamePalettes) {
	SceneGraph sceneGraph;
	palette::Palette pal;
	pal.nippon();
	voxel::RawVolume v(voxel::Region(0, 1));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("model1");
		node.setPalette(pal);
		EXPECT_GT(sceneGraph.emplace(core::move(node), 0), 0);
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("model2");
		node.setPalette(pal);
		EXPECT_GT(sceneGraph.emplace(core::move(node), 0), 0);
	}
	const palette::Palette &palette = sceneGraph.mergePalettes(true);
	ASSERT_EQ(palette.colorCount(), pal.colorCount()) << palette << pal;
	for (int i = 0; i < pal.colorCount(); ++i) {
		const color::RGBA c1 = palette.color(i);
		const color::RGBA c2 = pal.color(i);
		ASSERT_EQ(c1, c2) << "Color at index " << i << " differs: " << color::Color::print(c1, true) << " != " << color::Color::print(c2, true);
	}
	ASSERT_EQ(palette.hash(), pal.hash()) << palette << pal;
}

TEST_F(SceneGraphTest, testPaletteMergeTooManyColors) {
	SceneGraph sceneGraph;
	{
		palette::Palette pal;
		pal.nippon();
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setName("model");
		node.setPalette(pal);
		EXPECT_GT(sceneGraph.emplace(core::move(node), 0), 0);
	}
	{
		palette::Palette pal;
		pal.magicaVoxel();
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		node.setName("model2");
		node.setPalette(pal);
		EXPECT_GT(sceneGraph.emplace(core::move(node), 0), 0);
	}
	const palette::Palette &palette = sceneGraph.mergePalettes(true);
	ASSERT_EQ(palette.colorCount(), 2) << palette;
}

TEST_F(SceneGraphTest, testChildren) {
	SceneGraph sceneGraph;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("model");
		EXPECT_EQ(1, sceneGraph.emplace(core::move(node), 0))
			<< "Unexpected node id returned - root node is 0 - next should be 1";
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("group");
		EXPECT_EQ(2, sceneGraph.emplace(core::move(node), 1));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("model2");
		EXPECT_EQ(3, sceneGraph.emplace(core::move(node), 2));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("model");
		EXPECT_EQ(4, sceneGraph.emplace(core::move(node), 1));
	}
	EXPECT_EQ(1, sceneGraph.root().children()[0]);
	ASSERT_TRUE(sceneGraph.hasNode(1));
	const SceneGraphNode &modelNode = sceneGraph.node(1);
	EXPECT_EQ(SceneGraphNodeType::Model, modelNode.type());
	EXPECT_EQ(1, modelNode.id());
	EXPECT_EQ("model", modelNode.name());
	ASSERT_EQ(2u, modelNode.children().size());
	EXPECT_EQ(2, modelNode.children()[0]) << "First child should be the node with the id 2";
	ASSERT_TRUE(sceneGraph.hasNode(2));
	EXPECT_EQ(modelNode.id(), sceneGraph.node(2).parent());
	EXPECT_EQ(4, modelNode.children()[1]) << "Second child should be the node with the id 4";
	ASSERT_TRUE(sceneGraph.hasNode(4));
	EXPECT_EQ(modelNode.id(), sceneGraph.node(4).parent());
	EXPECT_EQ(3u, sceneGraph.size(SceneGraphNodeType::Model));
	ASSERT_EQ(1u, sceneGraph.root().children().size());
}

TEST_F(SceneGraphTest, testRemove) {
	SceneGraph sceneGraph;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("node");
		sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("children");
		sceneGraph.emplace(core::move(node), 1);
	}
	EXPECT_EQ(2u, sceneGraph.size(SceneGraphNodeType::Model));
	EXPECT_TRUE(sceneGraph.removeNode(1, true));
	EXPECT_EQ(0u, sceneGraph.size(SceneGraphNodeType::Model));
	EXPECT_TRUE(sceneGraph.empty(SceneGraphNodeType::Model));
}

TEST_F(SceneGraphTest, testMerge) {
	SceneGraph sceneGraph;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node1");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 1));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setVolume(v, true);
		sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node2");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(1, 2));
		v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		node.setVolume(v, true);
		sceneGraph.emplace(core::move(node));
	}
	EXPECT_EQ(2u, sceneGraph.size(SceneGraphNodeType::AllModels));
	SceneGraph::MergeResult merged = sceneGraph.merge();
	core::ScopedPtr<voxel::RawVolume> v(merged.volume());
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(3, v->region().getWidthInVoxels());
	EXPECT_TRUE(voxel::isBlocked(v->voxel(1, 1, 1).getMaterial()));
}

TEST_F(SceneGraphTest, testSceneOBB) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(2, 3));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.localTranslate(-v.region().calcCenterf());
		node.setPivot(glm::vec3(0.0f));
		ASSERT_EQ(1, sceneGraph.emplace(core::move(node)));
		sceneGraph.updateTransforms();
	}
	const math::OBBF &obb = sceneGraph.sceneOBB(sceneGraph.node(1), 0);
	const glm::vec3 &center = obb.origin();
	EXPECT_FLOAT_EQ(0.0f, center.x);
	EXPECT_FLOAT_EQ(0.0f, center.y);
	EXPECT_FLOAT_EQ(0.0f, center.z);

	const glm::vec3 &extents = obb.extents();
	EXPECT_FLOAT_EQ(1.0f, extents.x);
	EXPECT_FLOAT_EQ(1.0f, extents.y);
	EXPECT_FLOAT_EQ(1.0f, extents.z);
}

TEST_F(SceneGraphTest, DISABLED_testSceneOBBScale) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(2, 3));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.localTranslate(glm::vec3(-3.0f));
		node.transform(0).setLocalScale({2.0f, 2.0f, 2.0f});
		node.setPivot(glm::vec3(0.0f));
		ASSERT_EQ(1, sceneGraph.emplace(core::move(node)));
		sceneGraph.updateTransforms();
	}
	const math::OBBF &obb = sceneGraph.sceneOBB(sceneGraph.node(1), 0);
	const glm::vec3 &center = obb.origin();
	EXPECT_FLOAT_EQ(0.0f, center.x);
	EXPECT_FLOAT_EQ(0.0f, center.y);
	EXPECT_FLOAT_EQ(0.0f, center.z);

	const glm::vec3 &extents = obb.extents();
	EXPECT_FLOAT_EQ(2.0f, extents.x);
	EXPECT_FLOAT_EQ(2.0f, extents.y);
	EXPECT_FLOAT_EQ(2.0f, extents.z);
}

TEST_F(SceneGraphTest, testNodeSceneRegion_1) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(-3, 3));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.localTranslate(glm::vec3(10.0f, 11.0f, 12.0f));
		node.setPivot(glm::vec3(0.5f));
		ASSERT_EQ(1, sceneGraph.emplace(core::move(node)));
		sceneGraph.updateTransforms();
	}
	const voxel::Region &region = sceneGraph.sceneRegion(sceneGraph.node(1));
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	EXPECT_EQ(3, mins.x);
	EXPECT_EQ(4, mins.y);
	EXPECT_EQ(5, mins.z);
	EXPECT_EQ(10, maxs.x);
	EXPECT_EQ(11, maxs.y);
	EXPECT_EQ(12, maxs.z);
}

TEST_F(SceneGraphTest, testNodeSceneRegion_2) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 3));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.localTranslate(glm::vec3(10.0f, 11.0f, 12.0f));
		node.setPivot(glm::vec3(0.0f));
		ASSERT_EQ(1, sceneGraph.emplace(core::move(node)));
		sceneGraph.updateTransforms();
	}
	const voxel::Region &region = sceneGraph.sceneRegion(sceneGraph.node(1));
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	EXPECT_EQ(10, mins.x);
	EXPECT_EQ(11, mins.y);
	EXPECT_EQ(12, mins.z);
	EXPECT_EQ(13, maxs.x);
	EXPECT_EQ(14, maxs.y);
	EXPECT_EQ(15, maxs.z);
}

TEST_F(SceneGraphTest, testMergeWithTranslation) {
	SceneGraph sceneGraph;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node1");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 10));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setVolume(v, true);
		SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(-10));
		node.setTransform(0, transform);
		sceneGraph.emplace(core::move(node));
		// [-10, 0]
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node2");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(1, 5));
		v->setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		node.setVolume(v, true);
		SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(10));
		node.setTransform(0, transform);
		sceneGraph.emplace(core::move(node));
		// [11, 15]
	}
	sceneGraph.updateTransforms();
	SceneGraph::MergeResult merged = sceneGraph.merge();
	core::ScopedPtr<voxel::RawVolume> v(merged.volume());
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(26, v->region().getWidthInVoxels());
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-9, -9, -9).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(12, 12, 12).getMaterial()));
}

TEST_F(SceneGraphTest, testMergeWithTranslationAndPivot) {
	SceneGraph sceneGraph;
	const int expectedWidthNode1 = 11;
	const int expectedWidthNode2 = 5;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node1");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 10));
		EXPECT_EQ(expectedWidthNode1, v->region().getWidthInVoxels());
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setVolume(v, true);
		SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(-10));
		node.setTransform(0, transform);
		node.setPivot(glm::vec3(1.0f) / glm::vec3(v->region().getDimensionsInVoxels()));
		EXPECT_EQ(1, sceneGraph.emplace(core::move(node)));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node2");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(1, 5));
		EXPECT_EQ(expectedWidthNode2, v->region().getWidthInVoxels());
		v->setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		node.setVolume(v, true);
		SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(10));
		node.setTransform(0, transform);
		node.setPivot(glm::vec3(0.0f));
		EXPECT_EQ(2, sceneGraph.emplace(core::move(node)));
	}
	sceneGraph.updateTransforms();

	const voxel::Region &node1Region = sceneGraph.sceneRegion(sceneGraph.node(1));
	// without pivot it would be (-10, 0)
	EXPECT_EQ(voxel::Region(-11, -1), node1Region);

	const voxel::Region &node2Region = sceneGraph.sceneRegion(sceneGraph.node(2));
	EXPECT_EQ(voxel::Region(11, 15), node2Region);

	voxel::Region completeRegion = node1Region;
	completeRegion.accumulate(node2Region);

	SceneGraph::MergeResult merged = sceneGraph.merge();
	core::ScopedPtr<voxel::RawVolume> v(merged.volume());
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(completeRegion, v->region());
	EXPECT_EQ(27, v->region().getWidthInVoxels());
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-10, -10, -10).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(12, 12, 12).getMaterial()));
}

// TODO: SCENEGRAPH: implement rotation here
//       https://github.com/vengi-voxel/vengi/issues/433
TEST_F(SceneGraphTest, DISABLED_testMergeWithTranslationPivotAndRotation) {
	SceneGraph sceneGraph;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node1");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 10));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setVolume(v, true);
		const glm::vec3 euler(45.0f, 0.0f, 0.0f);
		SceneGraphTransform transform;
		transform.setWorldOrientation(glm::quat(euler));
		transform.setWorldTranslation(glm::vec3(-10));
		node.setTransform(0, transform);
		node.setPivot(glm::vec3(1.0f) / glm::vec3(v->region().getDimensionsInVoxels()));
		sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("node2");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(1, 5));
		v->setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		node.setVolume(v, true);
		const glm::vec3 euler(0.0f, 45.0f, 0.0f);
		SceneGraphTransform transform;
		transform.setWorldOrientation(glm::quat(euler));
		transform.setWorldTranslation(glm::vec3(10));
		node.setTransform(0, transform);
		node.setPivot(glm::vec3(0.0f));
		sceneGraph.emplace(core::move(node));
	}
	SceneGraph::MergeResult merged = sceneGraph.merge();
	core::ScopedPtr<voxel::RawVolume> v(merged.volume());
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(27, v->region().getWidthInVoxels());
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-10, -10, -10).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(12, 12, 12).getMaterial()));
}

TEST_F(SceneGraphTest, testKeyframes) {
	SceneGraphNode node(SceneGraphNodeType::Group);
	EXPECT_EQ(InvalidKeyFrame, node.addKeyFrame(0));
	for (int i = 0; i < 10; ++i) {
		EXPECT_EQ((scenegraph::KeyFrameIndex)0, node.keyFrameForFrame(i))
			<< "Failed to get the correct key frame for frame " << i;
	}
	const SceneGraphKeyFrames &kfs = *node.keyFrames();
	EXPECT_EQ(1u, kfs.size());
	EXPECT_NE(InvalidKeyFrame, node.addKeyFrame(6));
	for (int i = 6; i < 10; ++i) {
		EXPECT_EQ((scenegraph::KeyFrameIndex)1, node.keyFrameForFrame(i))
			<< "Failed to get the correct key frame for frame " << i;
	}
	EXPECT_EQ(2u, kfs.size());
	EXPECT_TRUE(node.removeKeyFrame(6));
	EXPECT_EQ(1u, kfs.size());
	EXPECT_NE(InvalidKeyFrame, node.addKeyFrame(6));
	EXPECT_TRUE(node.removeKeyFrame(8));
	EXPECT_EQ(1u, kfs.size());
}

TEST_F(SceneGraphTest, testMoveParentAsNewChild) {
	SceneGraph sceneGraph;
	int originalParentNodeId = 1;
	int originalChildNodeId = 2;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("originalparent");
		EXPECT_EQ(originalParentNodeId, sceneGraph.emplace(core::move(node), 0))
			<< "Unexpected node id returned - root node is 0 - next should be 1";
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("originalchild");
		EXPECT_EQ(originalChildNodeId, sceneGraph.emplace(core::move(node), originalParentNodeId));
	}
	ASSERT_FALSE(sceneGraph.changeParent(originalParentNodeId, originalChildNodeId));
}

TEST_F(SceneGraphTest, testMove) {
	SceneGraph sceneGraph;
	int originalParentNodeId = 1;
	int originalChildNodeId = 2;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("model1");
		EXPECT_EQ(originalParentNodeId, sceneGraph.emplace(core::move(node), 0))
			<< "Unexpected node id returned - root node is 0 - next should be 1";
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("model2");
		EXPECT_EQ(originalChildNodeId, sceneGraph.emplace(core::move(node), 0));
	}
	ASSERT_TRUE(sceneGraph.changeParent(originalParentNodeId, originalChildNodeId));
	ASSERT_EQ(1u, sceneGraph.root().children().size()) << "Expected to have one child after the move";
	ASSERT_EQ(originalChildNodeId, sceneGraph.root().children().front());
	const SceneGraphNode &newParentNode = sceneGraph.node(originalChildNodeId);
	ASSERT_EQ(1u, newParentNode.children().size());
	ASSERT_EQ(originalParentNodeId, newParentNode.children().front());
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const SceneGraphNode &node = *iter;
		EXPECT_FALSE(node.transform(0).dirty())
			<< "node " << node.name().c_str() << " still has a dirty transform after the move";
	}
}

TEST_F(SceneGraphTest, testAddKeyFrame) {
	SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	EXPECT_EQ(InvalidKeyFrame, node.addKeyFrame(0));
	EXPECT_EQ(1, node.addKeyFrame(10));
	EXPECT_EQ(2, node.addKeyFrame(20));
	EXPECT_EQ(InvalidKeyFrame, node.addKeyFrame(20));
}

TEST_F(SceneGraphTest, testAddKeyFrameValidateTranslate) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 0));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		sceneGraph.emplace(core::move(node));
	}
	ASSERT_FALSE(sceneGraph.animations().empty());
	ASSERT_TRUE(sceneGraph.setAnimation(sceneGraph.animations()[0]));

	{
		SceneGraphNode &node = sceneGraph.node(1);
		EXPECT_EQ(1, node.addKeyFrame(1));
		EXPECT_EQ(2, node.addKeyFrame(10));
		EXPECT_EQ(3, node.addKeyFrame(20));
		EXPECT_EQ(3, node.addKeyFrame(15)) << "Expected to insert a new key frame at index 3 (sorting by frameIdx)";
		EXPECT_EQ(5, node.addKeyFrame(30));
	}
}

TEST_F(SceneGraphTest, testKeyFrameTransformLerp) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 0));
	int firstNodeId;
	int secondNodeId;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("Parent");
		firstNodeId = sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("Child");
		secondNodeId = sceneGraph.emplace(core::move(node), firstNodeId);
	}
	ASSERT_FALSE(sceneGraph.animations().empty());
	ASSERT_TRUE(sceneGraph.setAnimation(sceneGraph.animations()[0]));
	{
		SceneGraphNode &parentNode1 = sceneGraph.node(firstNodeId);
		SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(100.0f, 0.0, 0.0f));
		transform.setWorldOrientation(glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f)));
		EXPECT_EQ(1, parentNode1.addKeyFrame(20)) << "Expected to get key frame index 1";
		parentNode1.keyFrame(1).setTransform(transform);
		sceneGraph.updateTransforms();
	}
	{
		const SceneGraphNode &parentNode1 = sceneGraph.node(firstNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(parentNode1, 20);
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		transform.decompose(scale, orientation, translation);
		EXPECT_FLOAT_EQ(translation.x, 100.0f);
		EXPECT_FLOAT_EQ(translation.x, 100.0f);
		EXPECT_FLOAT_EQ(glm::eulerAngles(orientation).x, glm::radians(90.0f));
	}
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 20);
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		transform.decompose(scale, orientation, translation);
		EXPECT_FLOAT_EQ(translation.x, 100.0f)
			<< "The child node should also get the world translation of the parent";
		EXPECT_FLOAT_EQ(glm::eulerAngles(orientation).x, glm::radians(90.0f));
	}
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 10);
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		transform.decompose(scale, orientation, translation);
		EXPECT_FLOAT_EQ(translation.x, 50.0f)
			<< "The child node should also get the world translation of the parent";
		EXPECT_FLOAT_EQ(glm::eulerAngles(orientation).x, glm::radians(45.0f));
	}
}

TEST_F(SceneGraphTest, testKeyFrameTransformParentRotation) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 0));
	int firstNodeId;
	int secondNodeId;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("Parent");
		SceneGraphTransform transform;
		transform.setWorldOrientation(glm::quat(glm::vec3(glm::radians(180.0f), 0.0f, 0.0f)));
		node.setTransform(0, transform);
		firstNodeId = sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("Child");
		secondNodeId = sceneGraph.emplace(core::move(node), firstNodeId);
	}
	sceneGraph.updateTransforms();
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 0);
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		transform.decompose(scale, orientation, translation);
		EXPECT_FLOAT_EQ(glm::abs(glm::eulerAngles(orientation).x), glm::radians(180.0f));
	}
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 1);
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		transform.decompose(scale, orientation, translation);
		EXPECT_FLOAT_EQ(glm::abs(glm::eulerAngles(orientation).x), glm::radians(180.0f));
	}
}

TEST_F(SceneGraphTest, testKeyFrameTransformParentRotation2) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 0));
	int firstNodeId;
	int secondNodeId;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("Parent");
		SceneGraphTransform transform0;
		transform0.setWorldOrientation(glm::quat(glm::vec3(glm::radians(-180.0f), 0.0f, 0.0f)));
		node.setTransform(0, transform0);
		node.keyFrame(1).frameIdx = 40;
		node.keyFrame(1).transform().setWorldOrientation(glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)));
		firstNodeId = sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.setName("Child");
		secondNodeId = sceneGraph.emplace(core::move(node), firstNodeId);
	}
	sceneGraph.updateTransforms();
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 0);
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		transform.decompose(scale, orientation, translation);
		EXPECT_FLOAT_EQ(glm::abs(glm::eulerAngles(orientation).x), glm::radians(180.0f));
	}
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 20);
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		transform.decompose(scale, orientation, translation);
		EXPECT_FLOAT_EQ(glm::abs(glm::eulerAngles(orientation).x), glm::radians(90.0f));
	}
}

TEST_F(SceneGraphTest, testSceneRegion) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(-3, 3));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.localTranslate(glm::vec3(10.0f, 11.0f, 12.0f));
		node.setPivot(glm::vec3(0.0f));
		sceneGraph.emplace(core::move(node));
	}
#if 0
	voxel::RawVolume v2(voxel::Region(-13, 13));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v2, false);
		node.translate(glm::vec3(1.0f, 1.0f, 2.0f));
		node.setPivot(glm::vec3(0.0f));
		sceneGraph.emplace(core::move(node));
	}
#endif
	sceneGraph.updateTransforms();
	const voxel::Region sceneRegion = sceneGraph.sceneRegion();
	const glm::ivec3 &mins = sceneRegion.getLowerCorner();
	const glm::ivec3 &maxs = sceneRegion.getUpperCorner();

	EXPECT_EQ(7, mins.x);
	EXPECT_EQ(8, mins.y);
	EXPECT_EQ(9, mins.z);
	EXPECT_EQ(13, maxs.x);
	EXPECT_EQ(14, maxs.y);
	EXPECT_EQ(15, maxs.z);
}

TEST_F(SceneGraphTest, testSetAnimation) {
	SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	EXPECT_TRUE(node.addAnimation("Test"));
	EXPECT_TRUE(node.addAnimation("Test2"));
	EXPECT_FALSE(node.addAnimation("Test"));
	EXPECT_TRUE(node.removeAnimation("Test2"));
	EXPECT_FALSE(node.removeAnimation("Test2"));
	EXPECT_TRUE(node.setAnimation("Test"));
	EXPECT_TRUE(node.setAnimation("Test2"));
	EXPECT_TRUE(node.removeAnimation("Test2"));
}

TEST_F(SceneGraphTest, testSceneGraphKeyFramesMap) {
	SceneGraphKeyFramesMap keyFramesMap;
	SceneGraphKeyFrames frames;
	frames.emplace_back(SceneGraphKeyFrame{});
	keyFramesMap.emplace("Test", core::move(frames));
}

TEST_F(SceneGraphTest, testChildRotationFlipping) {
	SceneGraph sceneGraph;
	int parentId = InvalidNodeId;
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("parent");
		SceneGraphTransform transform;
		// Rotate parent 90 degrees around Y
		transform.setLocalOrientation(glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		node.setTransform(0, transform);
		parentId = sceneGraph.emplace(core::move(node));
	}
	int childId = InvalidNodeId;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("child");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		SceneGraphTransform transform;
		// Rotate child 90 degrees around X
		transform.setLocalOrientation(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
		node.setTransform(0, transform);
		childId = sceneGraph.emplace(core::move(node), parentId);
	}

	sceneGraph.updateTransforms();
	const glm::mat4 worldMatrix = sceneGraph.worldMatrix(sceneGraph.node(childId), 0);
	// Expected: Parent Rotation * Child Rotation
	// R_p = RotY(90)
	// R_c = RotX(90)
	// World = R_p * R_c

	glm::quat qp = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat qc = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 expected = glm::mat4_cast(qp) * glm::mat4_cast(qc);

	// Check if they match
	// We can check basis vectors
	glm::vec3 expectedRight = glm::vec3(expected[0]);
	glm::vec3 expectedUp = glm::vec3(expected[1]);
	glm::vec3 expectedForward = glm::vec3(expected[2]);

	glm::vec3 actualRight = glm::vec3(worldMatrix[0]);
	glm::vec3 actualUp = glm::vec3(worldMatrix[1]);
	glm::vec3 actualForward = glm::vec3(worldMatrix[2]);

	EXPECT_NEAR(expectedRight.x, actualRight.x, 0.001f);
	EXPECT_NEAR(expectedRight.y, actualRight.y, 0.001f);
	EXPECT_NEAR(expectedRight.z, actualRight.z, 0.001f);
	EXPECT_NEAR(expectedUp.x, actualUp.x, 0.001f);
	EXPECT_NEAR(expectedUp.y, actualUp.y, 0.001f);
	EXPECT_NEAR(expectedUp.z, actualUp.z, 0.001f);
	EXPECT_NEAR(expectedForward.x, actualForward.x, 0.001f);
	EXPECT_NEAR(expectedForward.y, actualForward.y, 0.001f);
	EXPECT_NEAR(expectedForward.z, actualForward.z, 0.001f);
}

}
