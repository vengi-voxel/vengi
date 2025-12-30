/**
 * @file
 */

#include "scenegraph/SceneGraphUtil.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "scenegraph/SceneGraphNode.h"

namespace scenegraph {

class SceneGraphUtilTest : public app::AbstractTest {};

TEST_F(SceneGraphUtilTest, testAddSceneGraphNodes) {
	SceneGraph source;
	int groupNodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("group");
		groupNodeId = source.emplace(core::move(node));
	}
	{
		SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		source.emplace(core::move(node), groupNodeId);
	}
	SceneGraph target;
	EXPECT_EQ(1, addSceneGraphNodes(target, source, target.root().id()));
	ASSERT_TRUE(target.hasNode(1));
	EXPECT_EQ(SceneGraphNodeType::Group, target.node(1).type());
	ASSERT_TRUE(target.hasNode(2));
	EXPECT_EQ(SceneGraphNodeType::Model, target.node(2).type());
	ASSERT_EQ(1, target.node(2).parent());
}

TEST_F(SceneGraphUtilTest, testCopySceneGraphWithReferences) {
	SceneGraph source;
	int modelNodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		modelNodeId = source.emplace(core::move(node));
	}
	ASSERT_NE(InvalidNodeId, modelNodeId);
	{
		SceneGraphNode node(SceneGraphNodeType::ModelReference);
		node.setName("reference");
		node.setReference(modelNodeId);
		int refNodeId = source.emplace(core::move(node));
		ASSERT_NE(InvalidNodeId, refNodeId);
	}

	ASSERT_EQ(3u, source.nodeSize());
	ASSERT_EQ(2u, source.root().children().size());

	SceneGraph target;
	copySceneGraph(target, source);

	ASSERT_EQ(3u, target.nodeSize()); // Root + Model + Reference

	int targetModelId = -1;
	int targetRefId = -1;

	for (auto iter = target.begin(SceneGraphNodeType::Model); iter != target.end(); ++iter) {
		targetModelId = (*iter).id();
	}
	for (auto iter = target.begin(SceneGraphNodeType::ModelReference); iter != target.end(); ++iter) {
		targetRefId = (*iter).id();
	}

	ASSERT_NE(-1, targetModelId);
	ASSERT_NE(-1, targetRefId);

	const SceneGraphNode &targetRefNode = target.node(targetRefId);
	EXPECT_EQ(targetModelId, targetRefNode.reference());
}

TEST_F(SceneGraphUtilTest, testSplitVolumesWithReferences) {
	SceneGraph source;
	int modelNodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("model");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 1, 0, 0));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setVolume(v, true);
		modelNodeId = source.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::ModelReference);
		node.setName("reference");
		node.setReference(modelNodeId);
		source.emplace(core::move(node));
	}

	SceneGraph target;
	glm::ivec3 maxSize(1, 1, 1);
	splitVolumes(source, target, false, false, false, maxSize);

	int modelCount = 0;
	for (auto iter = target.begin(SceneGraphNodeType::Model); iter != target.end(); ++iter) {
		modelCount++;
	}
	EXPECT_EQ(2, modelCount);

	int refCount = 0;
	core::DynamicArray<int> referencedIds;
	for (auto iter = target.begin(SceneGraphNodeType::ModelReference); iter != target.end(); ++iter) {
		refCount++;
		referencedIds.push_back((*iter).reference());
	}
	EXPECT_EQ(2, refCount);
	EXPECT_EQ(2u, referencedIds.size());
	if (referencedIds.size() == 2) {
		EXPECT_NE(referencedIds[0], referencedIds[1]);
	}
}

TEST_F(SceneGraphUtilTest, testCopyNode) {
	SceneGraphNode src(SceneGraphNodeType::Model);
	src.setName("model");
	src.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
	src.setLocked(true);
	src.setVisible(false);

	SceneGraphNode target(SceneGraphNodeType::Model);
	copyNode(src, target, true);

	EXPECT_EQ(src.name(), target.name());
	EXPECT_EQ(src.locked(), target.locked());
	EXPECT_EQ(src.visible(), target.visible());
	EXPECT_NE(src.volume(), target.volume()); // Should be a copy
	EXPECT_NE(nullptr, target.volume());
}

TEST_F(SceneGraphUtilTest, testCopyNodeToSceneGraph) {
	SceneGraph source;
	int nodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		nodeId = source.emplace(core::move(node));
	}

	SceneGraph target;
	int newNodeId = copyNodeToSceneGraph(target, source.node(nodeId), target.root().id());
	ASSERT_NE(InvalidNodeId, newNodeId);
	EXPECT_TRUE(target.hasNode(newNodeId));
	EXPECT_EQ("model", target.node(newNodeId).name());
}

TEST_F(SceneGraphUtilTest, testMoveNodeToSceneGraph) {
	SceneGraph source;
	int nodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		nodeId = source.emplace(core::move(node));
	}

	SceneGraph target;
	int newNodeId = moveNodeToSceneGraph(target, source.node(nodeId), target.root().id());
	ASSERT_NE(InvalidNodeId, newNodeId);
	EXPECT_TRUE(target.hasNode(newNodeId));
	EXPECT_EQ("model", target.node(newNodeId).name());
	EXPECT_EQ(nullptr, source.node(nodeId).volume()); // Ownership transferred
}

TEST_F(SceneGraphUtilTest, testCreateNodeReference) {
	SceneGraph sceneGraph;
	int modelNodeId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		modelNodeId = sceneGraph.emplace(core::move(node));
	}

	int refNodeId = createNodeReference(sceneGraph, sceneGraph.node(modelNodeId));
	ASSERT_NE(InvalidNodeId, refNodeId);
	EXPECT_TRUE(sceneGraph.hasNode(refNodeId));
	EXPECT_EQ(SceneGraphNodeType::ModelReference, sceneGraph.node(refNodeId).type());
	EXPECT_EQ(modelNodeId, sceneGraph.node(refNodeId).reference());
}

TEST_F(SceneGraphUtilTest, testInterpolate) {
	EXPECT_DOUBLE_EQ(0.0, interpolate(InterpolationType::Linear, 0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(5.0, interpolate(InterpolationType::Linear, 5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(10.0, interpolate(InterpolationType::Linear, 10.0, 0.0, 10.0));

	// Instant (Step at 0.5)
	EXPECT_DOUBLE_EQ(0.0, interpolate(InterpolationType::Instant, 4.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(10.0, interpolate(InterpolationType::Instant, 6.0, 0.0, 10.0));

	// QuadEaseIn (t^2)
	// t = 0.5 -> 0.25. Value = 0 + 10 * 0.25 = 2.5
	EXPECT_DOUBLE_EQ(2.5, interpolate(InterpolationType::QuadEaseIn, 5.0, 0.0, 10.0));

	// QuadEaseOut (t * (2 - t))
	// t = 0.5 -> 0.5 * 1.5 = 0.75. Value = 0 + 10 * 0.75 = 7.5
	EXPECT_DOUBLE_EQ(7.5, interpolate(InterpolationType::QuadEaseOut, 5.0, 0.0, 10.0));

	// CatmullRom
	// t = 0.5 -> 5.0
	EXPECT_DOUBLE_EQ(5.0, interpolate(InterpolationType::CatmullRom, 5.0, 0.0, 10.0));
}

TEST_F(SceneGraphUtilTest, testSplitVolumesSkipHidden) {
	SceneGraph source;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("visible");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		source.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("hidden");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		node.setVisible(false);
		source.emplace(core::move(node));
	}

	SceneGraph target;
	glm::ivec3 maxSize(1, 1, 1);
	splitVolumes(source, target, false, false, true, maxSize);

	EXPECT_EQ(1u, target.size()); // Root + Visible
	EXPECT_EQ(1u, target.root().children().size());
	if (target.root().children().size() > 0) {
		EXPECT_EQ("visible", target.node(target.root().children()[0]).name());
	}
}

TEST_F(SceneGraphUtilTest, testSplitVolumesPreservesNodeTypes) {
	SceneGraph source;
	int groupId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Group);
		node.setName("group");
		groupId = source.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Camera);
		node.setName("camera");
		source.emplace(core::move(node), groupId);
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Point);
		node.setName("point");
		source.emplace(core::move(node), groupId);
	}
	int modelId = -1;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("model");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		modelId = source.emplace(core::move(node), groupId);
	}
	{
		SceneGraphNode node(SceneGraphNodeType::ModelReference);
		node.setName("reference");
		node.setReference(modelId);
		source.emplace(core::move(node), groupId);
	}

	SceneGraph target;
	glm::ivec3 maxSize(1, 1, 1);
	splitVolumes(source, target, false, false, false, maxSize);

	int groupCount = 0;
	for (auto iter = target.begin(SceneGraphNodeType::Group); iter != target.end(); ++iter) {
		groupCount++;
	}
	EXPECT_EQ(1, groupCount);

	int cameraCount = 0;
	for (auto iter = target.begin(SceneGraphNodeType::Camera); iter != target.end(); ++iter) {
		cameraCount++;
	}
	EXPECT_EQ(1, cameraCount);

	int pointCount = 0;
	for (auto iter = target.begin(SceneGraphNodeType::Point); iter != target.end(); ++iter) {
		pointCount++;
	}
	EXPECT_EQ(1, pointCount);

	int modelCount = 0;
	for (auto iter = target.begin(SceneGraphNodeType::Model); iter != target.end(); ++iter) {
		modelCount++;
	}
	EXPECT_EQ(1, modelCount);

	int refCount = 0;
	for (auto iter = target.begin(SceneGraphNodeType::ModelReference); iter != target.end(); ++iter) {
		refCount++;
	}
	EXPECT_EQ(1, refCount);
}

} // namespace scenegraph
