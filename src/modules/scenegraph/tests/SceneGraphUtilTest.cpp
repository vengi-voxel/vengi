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

} // namespace voxelformat
