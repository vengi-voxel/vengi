/**
 * @file
 */

#include "../MementoHandler.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"

namespace voxedit {

class MementoHandlerTest : public app::AbstractTest {
protected:
	voxedit::MementoHandler mementoHandler;
	core::SharedPtr<voxel::RawVolume> create(int size) const {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size - 1));
		EXPECT_EQ(size, region.getWidthInVoxels());
		return core::make_shared<voxel::RawVolume>(region);
	}
	void SetUp() override {
		ASSERT_TRUE(mementoHandler.init());
	}

	void TearDown() override {
		mementoHandler.shutdown();
	}
};

TEST_F(MementoHandlerTest, testMarkUndo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_FALSE(mementoHandler.canUndo());

	mementoHandler.markUndo(0, 0, "", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	EXPECT_FALSE(mementoHandler.canRedo())
		<< "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(mementoHandler.canUndo())
		<< "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)mementoHandler.stateSize());
	EXPECT_EQ(0, (int)mementoHandler.statePosition());

	mementoHandler.markUndo(0, 0, "", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(2, (int)mementoHandler.stateSize());
	EXPECT_EQ(1, (int)mementoHandler.statePosition());

	mementoHandler.markUndo(0, 0, "", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, (int)mementoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 0, "", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 0, "", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());

	const voxedit::MementoState &undoThird = mementoHandler.undo();
	ASSERT_TRUE(undoThird.hasVolumeData());
	EXPECT_EQ(2, undoThird.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(1, (int)mementoHandler.statePosition());

	voxedit::MementoState undoSecond = mementoHandler.undo();
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_FALSE(mementoHandler.canUndo());
	EXPECT_EQ(0, (int)mementoHandler.statePosition());

	const voxedit::MementoState &redoSecond = mementoHandler.redo();
	ASSERT_TRUE(redoSecond.hasVolumeData());
	EXPECT_EQ(2, redoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(1, (int)mementoHandler.statePosition());

	undoSecond = mementoHandler.undo();
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_FALSE(mementoHandler.canUndo());
	EXPECT_EQ(0, (int)mementoHandler.statePosition());

	const voxedit::MementoState &undoNotPossible = mementoHandler.undo();
	ASSERT_FALSE(undoNotPossible.hasVolumeData());
}

TEST_F(MementoHandlerTest, testUndoRedoDifferentNodes) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 0", scenegraph::SceneGraphNodeType::Model, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 1", scenegraph::SceneGraphNodeType::Model, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), 0);
	mementoHandler.markUndo(0, 2, "Node 2", scenegraph::SceneGraphNodeType::Model, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), 0);
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());

	voxedit::MementoState state;
	{
		// undo of adding node 2
		state = mementoHandler.undo();
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo of adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	EXPECT_FALSE(mementoHandler.canUndo());
	EXPECT_TRUE(mementoHandler.canRedo());
	{
		// redo adding node 1
		state = mementoHandler.redo();
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
		mementoHandler.markUndo(0, i, "", scenegraph::SceneGraphNodeType::Max, v.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	}
	EXPECT_EQ(4, (int)mementoHandler.stateSize());
	EXPECT_EQ(3, mementoHandler.statePosition());
	mementoHandler.undo();
	mementoHandler.undo();
	EXPECT_EQ(1, mementoHandler.statePosition());
	mementoHandler.markUndo(0, 4, "Node 4", scenegraph::SceneGraphNodeType::Model, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 0, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 1", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());

	MementoState state;

	{
		// undo of adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo modification in node 0
		state = mementoHandler.undo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		// redo modification in node 0
		state = mementoHandler.redo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo of adding node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeSimple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	mementoHandler.markUndo(0, 0, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 1", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	MementoState state;

	EXPECT_EQ(2, (int)mementoHandler.stateSize());
	EXPECT_EQ(1, mementoHandler.statePosition());

	{
		// undo adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_TRUE(mementoHandler.canRedo());
	}
	{
		// redo adding node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testDeleteNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	mementoHandler.markUndo(0, 0, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	mementoHandler.markUndo(0, 1, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		// undo adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo adding node 1
		state = mementoHandler.redo();
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
	mementoHandler.markUndo(0, 0, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 0, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}

	{
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		state = mementoHandler.redo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}

	{
		state = mementoHandler.redo();
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
	mementoHandler.markUndo(0, 0, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 0, "Node 1 Modified", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);

	EXPECT_EQ(4, (int)mementoHandler.stateSize());
	EXPECT_EQ(3, mementoHandler.statePosition());

	MementoState state;

	{
		// undo the deletion of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// undo the modification of node 0
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canUndo());
	}

	{
		// redo the modification of node 0
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ("Node 1 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}

	{
		// redo the add of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}

	{
		// redo the removal of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(3, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// redo the removal of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(3, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(mementoHandler.canUndo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeMultiple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 2, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		// undo the creation of node 2
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(mementoHandler.canUndo());
	}
	{
		// undo the creation of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canUndo());
	}
	{
		// redo the creation of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}
	{
		// redo the creation of node 2
		state = mementoHandler.redo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeEdit) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(), MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
	mementoHandler.markUndo(0, 1, "Node 2 Modified", scenegraph::SceneGraphNodeType::Max, third.get(), MementoType::Modification, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}
	{
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canUndo());
	}
	{
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}
	{
		state = mementoHandler.redo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canRedo());
	}
}

#if 0
// TODO
TEST_F(MementoHandlerTest, testSceneNodeRenamed) {
}

TEST_F(MementoHandlerTest, testSceneNodePaletteChange) {
}

TEST_F(MementoHandlerTest, testSceneNodeMove) {
}
#endif

} // namespace voxedit
