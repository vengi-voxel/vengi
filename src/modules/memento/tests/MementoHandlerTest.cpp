/**
 * @file
 */

#include "../MementoHandler.h"
#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"

namespace memento {

class MementoHandlerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	static core::String toFakeUUID(int id) {
		if (id == InvalidNodeId) {
			return core::String::Empty;
		}
		return core::string::toString(id);
	}

	class TestMementoHandler : public MementoHandler {
	private:
		using Super = MementoHandler;

	public:
		bool markUndo(int parentId, int nodeId, int referenceId, const core::String &name,
					  scenegraph::SceneGraphNodeType nodeType, const voxel::RawVolume *volume, MementoType type,
					  const voxel::Region &region = voxel::Region::InvalidRegion,
					  const glm::vec3 &pivot = glm::vec3(0.0f),
					  const scenegraph::SceneGraphKeyFramesMap &allKeyFrames = {}, const palette::Palette &palette = {},
					  const palette::NormalPalette &normalPalette = {},
					  const scenegraph::SceneGraphNodeProperties &properties = {}) {
			return Super::markUndo(toFakeUUID(parentId), toFakeUUID(nodeId), toFakeUUID(referenceId), name, nodeType,
								   volume, type, region, pivot, allKeyFrames, palette, normalPalette, properties);
		}
	};

	TestMementoHandler _mementoHandler;
	scenegraph::SceneGraph _sceneGraph;

	core::SharedPtr<voxel::RawVolume> create(int size) const {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size - 1));
		EXPECT_EQ(size, region.getWidthInVoxels());
		return core::make_shared<voxel::RawVolume>(region);
	}

	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(_mementoHandler.init());
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, "1");
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
		node.setName("Node name");
		_sceneGraph.emplace(core::move(node));
	}

	void TearDown() override {
		_mementoHandler.shutdown();
		_sceneGraph.clear();
		Super::TearDown();
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
							 MementoType::Modification);
	EXPECT_FALSE(_mementoHandler.canRedo())
		<< "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(_mementoHandler.canUndo())
		<< "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::Modification);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, (int)_mementoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	ASSERT_TRUE(_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, first.get(),
										 MementoType::Modification));
	ASSERT_TRUE(_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(),
										 MementoType::Modification));
	ASSERT_TRUE(_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(),
										 MementoType::Modification));

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
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Model, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2", scenegraph::SceneGraphNodeType::Model, third.get(),
							 MementoType::SceneNodeAdded);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	MementoState state;
	{
		// undo of adding node 2
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, core::string::toInt(state.nodeUUID));
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo of adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_TRUE(_mementoHandler.canRedo());
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
								 MementoType::Modification);
	}
	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());
	_mementoHandler.undo();
	_mementoHandler.undo();
	EXPECT_EQ(1, _mementoHandler.statePosition());
	_mementoHandler.markUndo(0, 4, InvalidNodeId, "Node 4", scenegraph::SceneGraphNodeType::Model, second.get(),
							 MementoType::SceneNodeAdded);
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	MementoState state;

	{
		// undo of adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo modification in node 0
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, core::string::toInt(state.nodeUUID));
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		// redo modification in node 0
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(0, core::string::toInt(state.nodeUUID));
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo of adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeSimple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	MementoState state;

	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, _mementoHandler.statePosition());

	{
		// undo adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
							 MementoType::Modification);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeRemoved);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(0, core::string::toInt(state.nodeUUID));
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(0, core::string::toInt(state.nodeUUID));
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeRemoved);

	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the deletion of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
		EXPECT_EQ(0, core::string::toInt(state.nodeUUID));
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}

	{
		// redo the modification of node 0
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(0, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 1 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the add of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the removal of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// redo the removal of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
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
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the creation of node 2
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(2, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		// redo the creation of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		// redo the creation of node 2
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(2, core::string::toInt(state.nodeUUID));
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
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Modified", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::Modification);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(1, core::string::toInt(state.nodeUUID));
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testSceneNodeRenamed) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	node->setName("Name after");
	_mementoHandler.markNodeRenamed(_sceneGraph, *node);
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
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	ASSERT_EQ(1, (int)_mementoHandler.stateSize());
	{
		ScopedMementoGroup mementoGroup(_mementoHandler, "test");
		node->volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_mementoHandler.markModification(_sceneGraph, *node, voxel::Region(0, 0, 0, 0, 0, 0));
		node->setName("Name after");
		_mementoHandler.markNodeRenamed(_sceneGraph, *node);
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
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	EXPECT_EQ("nippon", node->palette().name());
	const int colorCount = node->palette().colorCount();
	palette::Palette palette;
	palette.commandAndConquer();
	node->setPalette(palette);
	_mementoHandler.markPaletteChange(_sceneGraph, *node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	MementoState state = firstState(_mementoHandler.undo());
	ASSERT_EQ(state.palette.colorCount(), colorCount);
	EXPECT_EQ(state.palette.name(), "nippon");
}

TEST_F(MementoHandlerTest, testSceneNodeMove) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const core::String oldParent = _sceneGraph.uuid(node->parent());
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	const auto *parentNode = _sceneGraph.findNodeByUUID(_mementoHandler.stateGroup().states[0].parentUUID);
	ASSERT_TRUE(parentNode != nullptr);
	EXPECT_EQ(parentNode->id(), 0);

	int groupId;
	{
		scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
		group.setName("Group");
		groupId = _sceneGraph.emplace(core::move(group));
		ASSERT_NE(groupId, InvalidNodeId);
	}
	ASSERT_TRUE(_sceneGraph.changeParent(node->id(), groupId));
	_mementoHandler.markNodeMoved(_sceneGraph, *node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(_mementoHandler.stateGroup().states[0].parentUUID, _sceneGraph.node(groupId).uuid());

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(oldParent, stateUndo.parentUUID);

	EXPECT_TRUE(_mementoHandler.canRedo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(_sceneGraph.node(groupId).uuid(), stateRedo.parentUUID);
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
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	{
		scenegraph::SceneGraphTransform &transform = node->transform(0);
		transform.mirrorX();
		transform.update(_sceneGraph, *node, 0, false);
		mirrored = transform.localTranslation();
		node->setTransform(0, transform);
	}
	_mementoHandler.markNodeTransform(_sceneGraph, *node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(MementoType::SceneNodeKeyFrames, stateUndo.type);
	ASSERT_FALSE(stateUndo.keyFrames.empty());

	{
		_sceneGraph.setAllKeyFramesForNode(*node, stateUndo.keyFrames);
		const scenegraph::SceneGraphTransform &transform = node->transform(0);
		EXPECT_VEC_NEAR(transform.localTranslation(), initial, 0.0001f);
	}

	EXPECT_TRUE(_mementoHandler.canRedo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(MementoType::SceneNodeKeyFrames, stateRedo.type);
	ASSERT_FALSE(stateRedo.keyFrames.empty());

	{
		_sceneGraph.setAllKeyFramesForNode(*node, stateRedo.keyFrames);
		const scenegraph::SceneGraphTransform &transform = node->transform(0);
		EXPECT_VEC_NEAR(transform.localTranslation(), mirrored, 0.0001f);
	}
}

TEST_F(MementoHandlerTest, testAllAnimations) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());

	ASSERT_TRUE(_sceneGraph.addAnimation("foo"));
	EXPECT_EQ(2u, _sceneGraph.animations().size()) << _sceneGraph.animations();
	_mementoHandler.markAnimationAdded(_sceneGraph, "foo");
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());

	ASSERT_TRUE(_sceneGraph.removeAnimation("foo"));
	EXPECT_EQ(1u, _sceneGraph.animations().size()) << _sceneGraph.animations();
	_mementoHandler.markAnimationRemoved(_sceneGraph, "foo");
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(MementoType::SceneGraphAnimation, stateUndo.type);
	ASSERT_TRUE(stateUndo.stringList.hasValue());
	ASSERT_EQ(2u, stateUndo.stringList.value()->size());
	_sceneGraph.setAnimations(*stateUndo.stringList.value());

	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(MementoType::SceneGraphAnimation, stateRedo.type);
	ASSERT_TRUE(stateRedo.stringList.hasValue());
	ASSERT_EQ(1u, stateRedo.stringList.value()->size());
	_sceneGraph.setAnimations(*stateRedo.stringList.value());
}

} // namespace memento
