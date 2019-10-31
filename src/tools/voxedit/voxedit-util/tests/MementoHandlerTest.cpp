/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../MementoHandler.h"
#include "voxel/RawVolume.h"

namespace voxedit {

class MementoHandlerTest: public core::AbstractTest {
protected:
	voxel::RawVolume* create(int size) const {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size - 1));
		EXPECT_EQ(size, region.getWidthInVoxels());
		return new voxel::RawVolume(region);
	}
};

TEST_F(MementoHandlerTest, testMarkUndo) {
	voxel::RawVolume* first = create(1);
	voxel::RawVolume* second = create(2);
	voxel::RawVolume* third = create(3);
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_FALSE(mementoHandler.canUndo());

	mementoHandler.markUndo(0, "", first);
	EXPECT_FALSE(mementoHandler.canRedo()) << "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(mementoHandler.canUndo()) << "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)mementoHandler.stateSize());
	EXPECT_EQ(0, (int)mementoHandler.statePosition());

	mementoHandler.markUndo(0, "", second);
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(2, (int)mementoHandler.stateSize());
	EXPECT_EQ(1, (int)mementoHandler.statePosition());

	mementoHandler.markUndo(0, "", third);
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, (int)mementoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	voxel::RawVolume* first = create(1);
	voxel::RawVolume* second = create(2);
	voxel::RawVolume* third = create(3);
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "", first);
	mementoHandler.markUndo(0, "", second);
	mementoHandler.markUndo(0, "", third);

	const voxedit::MementoState& undoThird = mementoHandler.undo();
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

	const voxedit::MementoState& redoSecond = mementoHandler.redo();
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

	const voxedit::MementoState& undoNotPossible = mementoHandler.undo();
	ASSERT_FALSE(undoNotPossible.hasVolumeData());
}

TEST_F(MementoHandlerTest, testUndoRedoDifferentLayers) {
	voxel::RawVolume* first = create(1);
	voxel::RawVolume* second = create(2);
	voxel::RawVolume* third = create(3);
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "", first);
	mementoHandler.markUndo(1, "", second);
	mementoHandler.markUndo(2, "", third);
	EXPECT_TRUE(mementoHandler.canUndo());
	voxedit::MementoState undoState = mementoHandler.undo();
	EXPECT_EQ(1, undoState.layer);
	ASSERT_TRUE(undoState.hasVolumeData());
	EXPECT_EQ(2, undoState.dataRegion().getWidthInVoxels());
	undoState = mementoHandler.undo();
	EXPECT_EQ(0, undoState.layer);
	ASSERT_TRUE(undoState.hasVolumeData());
	EXPECT_EQ(1, undoState.dataRegion().getWidthInVoxels());
	undoState = mementoHandler.redo();
	EXPECT_EQ(1, undoState.layer);
	ASSERT_TRUE(undoState.hasVolumeData());
	EXPECT_EQ(2, undoState.dataRegion().getWidthInVoxels());
}

TEST_F(MementoHandlerTest, testMaxUndoStates) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	for (int i = 0; i < MementoHandler::MaxStates * 2; ++i) {
		mementoHandler.markUndo(i, "", create(1));
	}
	ASSERT_EQ(MementoHandler::MaxStates, (int)mementoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewLayer) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "Layer 1", create(1));
	mementoHandler.markUndo(0, "Layer 1 Modified", create(2));
	mementoHandler.markLayerAdded(1, "Layer 2", create(3));
	MementoState state;

	state = mementoHandler.undo();
	EXPECT_EQ(1, state.layer);
	ASSERT_FALSE(state.hasVolumeData());

	state = mementoHandler.redo();
	EXPECT_EQ(1, state.layer);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
}

TEST_F(MementoHandlerTest, testAddNewLayerSimple) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "Layer 1", create(1));
	mementoHandler.markLayerAdded(1, "Layer 2", create(2));
	MementoState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <--
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1 <--
	// volume2  | add | 2
	state = mementoHandler.undo();
	EXPECT_EQ(1, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <--
	state = mementoHandler.redo();
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	EXPECT_FALSE(mementoHandler.canRedo());
}

TEST_F(MementoHandlerTest, testDeleteLayer) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "Layer 1", create(1));
	voxel::RawVolume* v2 = create(2);
	mementoHandler.markLayerAdded(1, "Layer 2 Added", v2);
	mementoHandler.markLayerDeleted(1, "Layer 2 Deleted", v2);

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume2  | del | 3
	// null     | del | 4 <---
	EXPECT_EQ(5, (int)mementoHandler.stateSize());
	EXPECT_EQ(4, mementoHandler.statePosition());

	MementoState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume2  | del | 3 <---
	// null     | del | 4
	state = mementoHandler.undo();
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume2  | del | 3
	// null     | del | 4 <---
	state = mementoHandler.redo();
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_FALSE(state.hasVolumeData());
}

TEST_F(MementoHandlerTest, testAddNewLayerExt) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "Layer 1", create(1));
	mementoHandler.markUndo(0, "Layer 1 Modified", create(2));
	mementoHandler.markLayerAdded(1, "Layer 2 Added", create(3));

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3 <---
	EXPECT_EQ(4, (int)mementoHandler.stateSize());
	EXPECT_EQ(3, mementoHandler.statePosition());

	MementoState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2 <---
	// volume3  | add | 3
	state = mementoHandler.undo();
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_FALSE(state.hasVolumeData());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1 <---
	// null     | add | 2
	// volume3  | add | 3
	state = mementoHandler.undo();
	EXPECT_EQ(1, mementoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1 Modified", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3 <---
	state = mementoHandler.redo();
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
}

TEST_F(MementoHandlerTest, testDeleteLayerExt) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "Layer 1", create(1));
	mementoHandler.markUndo(0, "Layer 1 Modified", create(2));
	voxel::RawVolume* v3 = create(3);
	mementoHandler.markLayerAdded(1, "Layer 2 Added", v3);
	mementoHandler.markLayerDeleted(1, "Layer 2 Deleted", v3);

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5 <---
	EXPECT_EQ(6, (int)mementoHandler.stateSize());
	EXPECT_EQ(5, mementoHandler.statePosition());

	MementoState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4 <---
	// null     | del | 5
	state = mementoHandler.undo();
	EXPECT_EQ(4, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2 <---
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = mementoHandler.undo();
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1 <---
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = mementoHandler.undo();
	EXPECT_EQ(1, mementoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1 Modified", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0 <---
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = mementoHandler.undo();
	EXPECT_EQ(0, mementoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	EXPECT_FALSE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1 <---
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = mementoHandler.redo();
	EXPECT_EQ(1, mementoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1 Modified", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3 <---
	// volume3  | del | 4
	// null     | del | 5
	state = mementoHandler.redo();
	EXPECT_EQ(3, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5 <---
	state = mementoHandler.redo();
	EXPECT_EQ(5, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_FALSE(mementoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4 <---
	// null     | del | 5
	state = mementoHandler.undo();
	EXPECT_EQ(4, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5 <---
	state = mementoHandler.redo();
	EXPECT_EQ(5, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_FALSE(mementoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4 <---
	// null     | del | 5
	state = mementoHandler.undo();
	EXPECT_EQ(4, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2 <---
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = mementoHandler.undo();
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_TRUE(mementoHandler.canUndo());
}

TEST_F(MementoHandlerTest, testAddNewLayerMultiple) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "Layer 1", create(1));
	mementoHandler.markLayerAdded(1, "Layer 2 Added", create(2));
	mementoHandler.markLayerAdded(2, "Layer 3 Added", create(3));

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4 <---
	EXPECT_EQ(5, (int)mementoHandler.stateSize());
	EXPECT_EQ(4, mementoHandler.statePosition());

	MementoState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3 <---
	// volume3  | add | 4
	state = mementoHandler.undo();
	EXPECT_EQ(3, mementoHandler.statePosition());
	EXPECT_EQ(2, state.layer);
	EXPECT_EQ("Layer 3 Added", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1 <---
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4
	state = mementoHandler.undo();
	EXPECT_EQ(1, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0 <---
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4
	state = mementoHandler.undo();
	EXPECT_EQ(0, mementoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	EXPECT_FALSE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <---
	// null     | add | 3
	// volume3  | add | 4
	state = mementoHandler.redo();
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4 <---
	state = mementoHandler.redo();
	EXPECT_EQ(4, mementoHandler.statePosition());
	EXPECT_EQ(2, state.layer);
	EXPECT_EQ("Layer 3 Added", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	EXPECT_FALSE(mementoHandler.canRedo());
}

TEST_F(MementoHandlerTest, testAddNewLayerEdit) {
	voxedit::MementoHandler mementoHandler;
	ASSERT_TRUE(mementoHandler.init());
	mementoHandler.markUndo(0, "Layer 1", create(1));
	mementoHandler.markLayerAdded(1, "Layer 2 Added", create(2));
	mementoHandler.markUndo(1, "Layer 2 Modified", create(3));

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume3  | mod | 3 <---
	EXPECT_EQ(4, (int)mementoHandler.stateSize());
	EXPECT_EQ(3, mementoHandler.statePosition());

	MementoState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <---
	// volume3  | mod | 3
	state = mementoHandler.undo();
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1 <---
	// volume2  | add | 2
	// volume3  | mod | 3
	state = mementoHandler.undo();
	EXPECT_EQ(1, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_FALSE(state.hasVolumeData());
	EXPECT_TRUE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0 <---
	// null     | add | 1
	// volume2  | add | 2
	// volume3  | mod | 3
	state = mementoHandler.undo();
	EXPECT_EQ(0, mementoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	EXPECT_FALSE(mementoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <---
	// volume3  | mod | 3
	state = mementoHandler.redo();
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume3  | mod | 3 <---
	state = mementoHandler.redo();
	EXPECT_EQ(3, mementoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Modified", state.name);
	ASSERT_TRUE(state.hasVolumeData());
	EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	EXPECT_FALSE(mementoHandler.canRedo());
}

}
