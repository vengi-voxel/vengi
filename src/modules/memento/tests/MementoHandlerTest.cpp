/**
 * @file
 */

#include "../MementoHandler.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"

namespace memento {

class MementoHandlerTest : public app::AbstractTest {
protected:
	MementoHandler _mementoHandler;
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

	static inline MementoState firstState(const MementoStateGroup &group) {
		core_assert(!group.states.empty());
		return group.states[0];
	}
};

TEST_F(MementoHandlerTest, testMarkUndo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	EXPECT_FALSE(_mementoHandler.canRedo())
		<< "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(_mementoHandler.canUndo())
		<< "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, (int)_mementoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	const MementoState &undoThird = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoThird.hasVolumeData());
	EXPECT_EQ(2, undoThird.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	MementoState undoSecond = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	const MementoState &redoSecond = firstState(_mementoHandler.redo());
	ASSERT_TRUE(redoSecond.hasVolumeData());
	EXPECT_EQ(2, redoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	undoSecond = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	const MementoStateGroup &undoNotPossibleGroup = _mementoHandler.undo();
	ASSERT_TRUE(undoNotPossibleGroup.states.empty());
}

TEST_F(MementoHandlerTest, testUndoRedoDifferentNodes) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Model, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Model, second.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), 0);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2", scenegraph::SceneGraphNodeType::Model, third.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), 0);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	MementoState state;
	{
		// undo of adding node 2
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo of adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_TRUE(_mementoHandler.canRedo());
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testCutStates) {
	core::SharedPtr<voxel::RawVolume> second = create(2);
	for (int i = 0; i < 4; ++i) {
		auto v = create(1);
		_mementoHandler.markUndo(0, i, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, v.get(),
								 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f),
								 glm::mat4(1.0f), InvalidKeyFrame);
	}
	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());
	_mementoHandler.undo();
	_mementoHandler.undo();
	EXPECT_EQ(1, _mementoHandler.statePosition());
	_mementoHandler.markUndo(0, 4, InvalidNodeId, "Node 4", scenegraph::SceneGraphNodeType::Model, second.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	MementoState state;

	{
		// undo of adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo modification in node 0
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		// redo modification in node 0
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo of adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, state.nodeId);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeSimple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);
	MementoState state;

	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, _mementoHandler.statePosition());

	{
		// undo adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1", state.name);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canUndo());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testDeleteNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeExt) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testDeleteNodeExt) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the deletion of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the modification of node 0
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}

	{
		// redo the modification of node 0
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ("Node 1 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the add of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the removal of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// redo the removal of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
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
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the creation of node 2
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		// redo the creation of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		// redo the creation of node 2
		state = firstState(_mementoHandler.redo());
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
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::vec3(0.0f),
							 glm::mat4(1.0f), InvalidKeyFrame);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Modified", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::Modification, voxel::Region::InvalidRegion, glm::vec3(0.0f), glm::mat4(1.0f),
							 InvalidKeyFrame);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		state = firstState(_mementoHandler.redo());
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
	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(stateUndo.name, "Node name");
	EXPECT_FALSE(_mementoHandler.canUndo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(stateRedo.name, "Name after");
}

TEST_F(MementoHandlerTest, testMementoGroupModificationRename) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(*node);
	ASSERT_EQ(1, (int)_mementoHandler.stateSize());
	{
		ScopedMementoGroup mementoGroup(_mementoHandler, "test");
		node->volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_mementoHandler.markModification(*node, voxel::Region(0, 0, 0, 0, 0, 0));
		node->setName("Name after");
		_mementoHandler.markNodeRenamed(*node);
	}
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	MementoState state = firstState(_mementoHandler.undo());
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
	MementoState state = firstState(_mementoHandler.undo());
	ASSERT_TRUE(state.palette.hasValue());
	EXPECT_EQ(state.palette.value()->name(), "nippon");
}

TEST_F(MementoHandlerTest, testSceneNodeMove) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const int oldParent = node->parent();
	_mementoHandler.markInitialNodeState(*node);
	EXPECT_EQ(_mementoHandler.stateGroup().states[0].parentId, 0);

	const int groupId = 2;
	_mementoHandler.markNodeMoved(groupId, node->id());
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(_mementoHandler.stateGroup().states[0].parentId, groupId);

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(oldParent, stateUndo.parentId);

	EXPECT_TRUE(_mementoHandler.canRedo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(groupId, stateRedo.parentId);
}

TEST_F(MementoHandlerTest, testSceneNodeTransform) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const glm::vec3 initial(1.0f, 2.0f, 3.0f);
	glm::vec3 mirrored;
	{
		scenegraph::SceneGraphTransform transform;
		transform.setLocalTranslation(initial);
		transform.update(_sceneGraph, *node, 0, false);
		node->setTransform(0, transform);
	}
	_mementoHandler.markInitialNodeState(*node);
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	{
		scenegraph::SceneGraphTransform &transform = node->transform(0);
		transform.mirrorX();
		transform.update(_sceneGraph, *node, 0, false);
		mirrored = transform.localTranslation();
		node->setTransform(0, transform);
	}
	_mementoHandler.markNodeTransform(*node, 0);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(MementoType::SceneNodeTransform, stateUndo.type);
	EXPECT_EQ(0, stateUndo.keyFrameIdx);
	ASSERT_TRUE(stateUndo.localMatrix.hasValue());

	{
		scenegraph::SceneGraphTransform transform;
		transform.setLocalMatrix(*stateUndo.localMatrix.value());
		transform.update(_sceneGraph, *node, 0, true);
		EXPECT_VEC_NEAR(transform.localTranslation(), initial, 0.0001f);
	}

	EXPECT_TRUE(_mementoHandler.canRedo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(0, stateRedo.keyFrameIdx);
	EXPECT_EQ(MementoType::SceneNodeTransform, stateRedo.type);
	ASSERT_TRUE(stateRedo.localMatrix.hasValue());

	{
		scenegraph::SceneGraphTransform transform;
		transform.setLocalMatrix(*stateRedo.localMatrix.value());
		transform.update(_sceneGraph, *node, 0, true);
		EXPECT_VEC_NEAR(transform.localTranslation(), mirrored, 0.0001f);
	}
}

} // namespace memento
