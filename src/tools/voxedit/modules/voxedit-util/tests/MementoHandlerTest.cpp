/**
 * @file
 */

#include "../MementoHandler.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"

namespace voxedit {

class MementoHandlerTest : public app::AbstractTest {
protected:
	voxedit::MementoHandler _mementoHandler;
	scenegraph::SceneGraph _sceneGraph;

	core::SharedPtr<voxel::RawVolume> create(int size) const {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size - 1));
		EXPECT_EQ(size, region.getWidthInVoxels());
		return core::make_shared<voxel::RawVolume>(region);
	}

	void SetUp() override {
		ASSERT_TRUE(_mementoHandler.init());
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("Node name");
		_sceneGraph.emplace(core::move(node));
	}

	void TearDown() override {
		_mementoHandler.shutdown();
		_sceneGraph.clear();
	}
};

TEST_F(MementoHandlerTest, testMarkUndo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	EXPECT_FALSE(_mementoHandler.canRedo())
		<< "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(_mementoHandler.canUndo())
		<< "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, (int)_mementoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	const voxedit::MementoState &undoThird = _mementoHandler.undo();
	ASSERT_TRUE(undoThird.hasVolumeData());
	EXPECT_EQ(2, undoThird.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	voxedit::MementoState undoSecond = _mementoHandler.undo();
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	const voxedit::MementoState &redoSecond = _mementoHandler.redo();
	ASSERT_TRUE(redoSecond.hasVolumeData());
	EXPECT_EQ(2, redoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	undoSecond = _mementoHandler.undo();
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	const voxedit::MementoState &undoNotPossible = _mementoHandler.undo();
	ASSERT_FALSE(undoNotPossible.hasVolumeData());
}

TEST_F(MementoHandlerTest, testUndoRedoDifferentNodes) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Model, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Model, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), 0);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2", scenegraph::SceneGraphNodeType::Model, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), 0);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	voxedit::MementoState state;
	{
		// undo of adding node 2
		state = _mementoHandler.undo();
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo of adding node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_TRUE(_mementoHandler.canRedo());
	{
		// redo adding node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testCutStates) {
	core::SharedPtr<voxel::RawVolume> second = create(2);
	for (int i = 0; i < 4; ++i) {
		auto v = create(1);
		_mementoHandler.markUndo(0, i, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, v.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	}
	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());
	_mementoHandler.undo();
	_mementoHandler.undo();
	EXPECT_EQ(1, _mementoHandler.statePosition());
	_mementoHandler.markUndo(0, 4, InvalidNodeId, "Node 4", scenegraph::SceneGraphNodeType::Model, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	MementoState state;

	{
		// undo of adding node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo modification in node 0
		state = _mementoHandler.undo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		// redo modification in node 0
		state = _mementoHandler.redo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo of adding node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeSimple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	MementoState state;

	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, _mementoHandler.statePosition());

	{
		// undo adding node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canUndo());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		// redo adding node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testDeleteNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo adding node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo adding node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeExt) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = _mementoHandler.undo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}

	{
		state = _mementoHandler.undo();
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		state = _mementoHandler.redo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}

	{
		state = _mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testDeleteNodeExt) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1 Modified", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the deletion of node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the modification of node 0
		state = _mementoHandler.undo();
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}

	{
		// redo the modification of node 0
		state = _mementoHandler.redo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ("Node 1 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the add of node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the removal of node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// redo the removal of node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeMultiple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the creation of node 2
		state = _mementoHandler.undo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		// undo the creation of node 1
		state = _mementoHandler.undo();
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		// redo the creation of node 1
		state = _mementoHandler.redo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		// redo the creation of node 2
		state = _mementoHandler.redo();
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeEdit) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Modified", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = _mementoHandler.undo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		state = _mementoHandler.undo();
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		state = _mementoHandler.redo();
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		state = _mementoHandler.redo();
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testSceneNodeRenamed) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(*node);
	node->setName("Name after");
	_mementoHandler.markNodeRenamed(*node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_TRUE(_mementoHandler.canUndo());
	const MementoState &stateUndo = _mementoHandler.undo();
	EXPECT_EQ(stateUndo.name, "Node name");
	EXPECT_FALSE(_mementoHandler.canUndo());
	const MementoState &stateRedo = _mementoHandler.redo();
	EXPECT_EQ(stateRedo.name, "Name after");
}

TEST_F(MementoHandlerTest, testMementoGroupModificationRename) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(*node);
	{
		ScopedMementoGroup group(_mementoHandler);
		node->volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_mementoHandler.markModification(*node, voxel::Region(0, 0, 0, 0, 0, 0));
		node->setName("Name after");
		_mementoHandler.markNodeRenamed(*node);
	}
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	const MementoState &state = _mementoHandler.undo();
	EXPECT_EQ(state.name, "Node name");
	voxel::RawVolume volume(voxel::Region(0, 0));
	ASSERT_TRUE(state.data.toVolume(&volume, state.data));
	EXPECT_EQ(voxel::VoxelType::Air, volume.voxel(0, 0, 0).getMaterial());
}

TEST_F(MementoHandlerTest, testSceneNodePaletteChange) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(*node);
	EXPECT_EQ("nippon", node->palette().name());
	palette::Palette palette;
	palette.commandAndConquer();
	node->setPalette(palette);
	_mementoHandler.markPaletteChange(*node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	const MementoState &state = _mementoHandler.undo();
	ASSERT_TRUE(state.palette.hasValue());
	EXPECT_EQ(state.palette.value()->name(), "nippon");
}

TEST_F(MementoHandlerTest, testSceneNodeMove) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const int oldParent = node->parent();
	_mementoHandler.markInitialNodeState(*node);
	EXPECT_EQ(_mementoHandler.state().parentId, 0);

	const int groupId = 2;
	_mementoHandler.markNodeMoved(groupId, node->id());
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(_mementoHandler.state().parentId, groupId);

	const MementoState &stateUndo = _mementoHandler.undo();
	EXPECT_EQ(oldParent, stateUndo.parentId);

	EXPECT_TRUE(_mementoHandler.canRedo());
	const MementoState &stateRedo = _mementoHandler.redo();
	EXPECT_EQ(groupId, stateRedo.parentId);
}

} // namespace voxedit
