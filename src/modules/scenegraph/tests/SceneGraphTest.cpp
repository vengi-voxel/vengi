/**
 * @file
 */

#include "scenegraph/SceneGraph.h"
#include "TestHelper.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/gtc/quaternion.hpp>

namespace scenegraph {

class SceneGraphTest : public app::AbstractTest {};

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
	SceneGraph::MergedVolumePalette merged = sceneGraph.merge(true);
	ASSERT_NE(nullptr, merged.first);
	EXPECT_EQ(3, merged.first->region().getWidthInVoxels());
	delete merged.first;
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
	SceneGraphNode node;
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
		EXPECT_FLOAT_EQ(transform.translation.x, 100.0f);
		EXPECT_FLOAT_EQ(transform.translation.x, 100.0f);
		EXPECT_FLOAT_EQ(glm::eulerAngles(transform.orientation).x, glm::radians(90.0f));
	}
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 20);
		EXPECT_FLOAT_EQ(transform.translation.x, 100.0f)
			<< "The child node should also get the world translation of the parent";
		EXPECT_FLOAT_EQ(glm::eulerAngles(transform.orientation).x, glm::radians(90.0f));
	}
	{
		const SceneGraphNode &childNode2 = sceneGraph.node(secondNodeId);
		const FrameTransform &transform = sceneGraph.transformForFrame(childNode2, 10);
		EXPECT_FLOAT_EQ(transform.translation.x, 50.0f)
			<< "The child node should also get the world translation of the parent";
		EXPECT_FLOAT_EQ(glm::eulerAngles(transform.orientation).x, glm::radians(45.0f));
	}
}

TEST_F(SceneGraphTest, testSceneRegion) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(-3, 3));
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(&v, false);
		node.translate(glm::vec3(10.0f, 11.0f, 12.0f));
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

} // namespace scenegraph
