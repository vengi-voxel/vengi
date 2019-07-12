/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../MementoHandler.h"
#include "voxel/polyvox/RawVolume.h"

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
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());

	undoHandler.markUndo(0, "", first);
	EXPECT_FALSE(undoHandler.canRedo()) << "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(undoHandler.canUndo()) << "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)undoHandler.stateSize());
	EXPECT_EQ(0, (int)undoHandler.statePosition());

	undoHandler.markUndo(0, "", second);
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(2, (int)undoHandler.stateSize());
	EXPECT_EQ(1, (int)undoHandler.statePosition());

	undoHandler.markUndo(0, "", third);
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(3, (int)undoHandler.stateSize());
	EXPECT_EQ(2, (int)undoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	voxel::RawVolume* first = create(1);
	voxel::RawVolume* second = create(2);
	voxel::RawVolume* third = create(3);
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "", first);
	undoHandler.markUndo(0, "", second);
	undoHandler.markUndo(0, "", third);

	const voxedit::LayerState& undoThird = undoHandler.undo();
	ASSERT_NE(nullptr, undoThird.volume);
	EXPECT_EQ(2, undoThird.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(1, (int)undoHandler.statePosition());

	voxedit::LayerState undoSecond = undoHandler.undo();
	ASSERT_NE(nullptr, undoSecond.volume);
	EXPECT_EQ(1, undoSecond.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());
	EXPECT_EQ(0, (int)undoHandler.statePosition());

	const voxedit::LayerState& redoSecond = undoHandler.redo();
	ASSERT_NE(nullptr, redoSecond.volume);
	EXPECT_EQ(2, redoSecond.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(1, (int)undoHandler.statePosition());

	undoSecond = undoHandler.undo();
	ASSERT_NE(nullptr, undoSecond.volume);
	EXPECT_EQ(1, undoSecond.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());
	EXPECT_EQ(0, (int)undoHandler.statePosition());

	const voxedit::LayerState& undoNotPossible = undoHandler.undo();
	ASSERT_EQ(nullptr, undoNotPossible.volume);
}

TEST_F(MementoHandlerTest, testUndoRedoDifferentLayers) {
	voxel::RawVolume* first = create(1);
	voxel::RawVolume* second = create(2);
	voxel::RawVolume* third = create(3);
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "", first);
	undoHandler.markUndo(1, "", second);
	undoHandler.markUndo(2, "", third);
	EXPECT_TRUE(undoHandler.canUndo());
	voxedit::LayerState undoState = undoHandler.undo();
	EXPECT_EQ(1, undoState.layer);
	ASSERT_NE(nullptr, undoState.volume);
	EXPECT_EQ(2, undoState.volume->region.getWidthInVoxels());
	undoState = undoHandler.undo();
	EXPECT_EQ(0, undoState.layer);
	ASSERT_NE(nullptr, undoState.volume);
	EXPECT_EQ(1, undoState.volume->region.getWidthInVoxels());
	undoState = undoHandler.redo();
	EXPECT_EQ(1, undoState.layer);
	ASSERT_NE(nullptr, undoState.volume);
	EXPECT_EQ(2, undoState.volume->region.getWidthInVoxels());
}

TEST_F(MementoHandlerTest, testMaxUndoStates) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	for (int i = 0; i < MementoHandler::MaxStates * 2; ++i) {
		undoHandler.markUndo(i, "", create(1));
	}
	ASSERT_EQ(MementoHandler::MaxStates, (int)undoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewLayer) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "Layer 1", create(1));
	undoHandler.markUndo(0, "Layer 1 Modified", create(2));
	undoHandler.markLayerAdded(1, "Layer 2", create(3));
	LayerState state;

	state = undoHandler.undo();
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ(nullptr, state.volume);

	state = undoHandler.redo();
	EXPECT_EQ(1, state.layer);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
}

TEST_F(MementoHandlerTest, testAddNewLayerSimple) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "Layer 1", create(1));
	undoHandler.markLayerAdded(1, "Layer 2", create(2));
	LayerState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <--
	EXPECT_EQ(3, (int)undoHandler.stateSize());
	EXPECT_EQ(2, undoHandler.statePosition());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1 <--
	// volume2  | add | 2
	state = undoHandler.undo();
	EXPECT_EQ(1, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2", state.name);
	EXPECT_EQ(nullptr, state.volume);
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <--
	state = undoHandler.redo();
	EXPECT_EQ(2, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());
	EXPECT_FALSE(undoHandler.canRedo());
}

TEST_F(MementoHandlerTest, testDeleteLayer) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "Layer 1", create(1));
	voxel::RawVolume* v2 = create(2);
	undoHandler.markLayerAdded(1, "Layer 2 Added", v2);
	undoHandler.markLayerDeleted(1, "Layer 2 Deleted", v2);

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume2  | del | 3
	// null     | del | 4 <---
	EXPECT_EQ(5, (int)undoHandler.stateSize());
	EXPECT_EQ(4, undoHandler.statePosition());

	LayerState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume2  | del | 3 <---
	// null     | del | 4
	state = undoHandler.undo();
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume2  | del | 3
	// null     | del | 4 <---
	state = undoHandler.redo();
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	EXPECT_EQ(nullptr, state.volume);
}

TEST_F(MementoHandlerTest, testAddNewLayerExt) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "Layer 1", create(1));
	undoHandler.markUndo(0, "Layer 1 Modified", create(2));
	undoHandler.markLayerAdded(1, "Layer 2 Added", create(3));

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3 <---
	EXPECT_EQ(4, (int)undoHandler.stateSize());
	EXPECT_EQ(3, undoHandler.statePosition());

	LayerState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2 <---
	// volume3  | add | 3
	state = undoHandler.undo();
	EXPECT_EQ(2, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	EXPECT_EQ(nullptr, state.volume);

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1 <---
	// null     | add | 2
	// volume3  | add | 3
	state = undoHandler.undo();
	EXPECT_EQ(1, undoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1 Modified", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3 <---
	state = undoHandler.redo();
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
}

TEST_F(MementoHandlerTest, testDeleteLayerExt) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "Layer 1", create(1));
	undoHandler.markUndo(0, "Layer 1 Modified", create(2));
	voxel::RawVolume* v3 = create(3);
	undoHandler.markLayerAdded(1, "Layer 2 Added", v3);
	undoHandler.markLayerDeleted(1, "Layer 2 Deleted", v3);

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5 <---
	EXPECT_EQ(6, (int)undoHandler.stateSize());
	EXPECT_EQ(5, undoHandler.statePosition());

	LayerState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4 <---
	// null     | del | 5
	state = undoHandler.undo();
	EXPECT_EQ(4, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2 <---
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = undoHandler.undo();
	EXPECT_EQ(2, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	EXPECT_EQ(nullptr, state.volume);
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1 <---
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = undoHandler.undo();
	EXPECT_EQ(1, undoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1 Modified", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0 <---
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = undoHandler.undo();
	EXPECT_EQ(0, undoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(1, state.volume->region.getWidthInVoxels());
	EXPECT_FALSE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1 <---
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = undoHandler.redo();
	EXPECT_EQ(1, undoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1 Modified", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3 <---
	// volume3  | del | 4
	// null     | del | 5
	state = undoHandler.redo();
	EXPECT_EQ(3, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5 <---
	state = undoHandler.redo();
	EXPECT_EQ(5, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_EQ(nullptr, state.volume);
	EXPECT_FALSE(undoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4 <---
	// null     | del | 5
	state = undoHandler.undo();
	EXPECT_EQ(4, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5 <---
	state = undoHandler.redo();
	EXPECT_EQ(5, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_EQ(nullptr, state.volume);
	EXPECT_FALSE(undoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2
	// volume3  | add | 3
	// volume3  | del | 4 <---
	// null     | del | 5
	state = undoHandler.undo();
	EXPECT_EQ(4, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Deleted", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// volume2  | mod | 1
	// null     | add | 2 <---
	// volume3  | add | 3
	// volume3  | del | 4
	// null     | del | 5
	state = undoHandler.undo();
	EXPECT_EQ(2, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	EXPECT_EQ(nullptr, state.volume);
	EXPECT_TRUE(undoHandler.canUndo());
}

TEST_F(MementoHandlerTest, testAddNewLayerMultiple) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "Layer 1", create(1));
	undoHandler.markLayerAdded(1, "Layer 2 Added", create(2));
	undoHandler.markLayerAdded(2, "Layer 3 Added", create(3));

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4 <---
	EXPECT_EQ(5, (int)undoHandler.stateSize());
	EXPECT_EQ(4, undoHandler.statePosition());

	LayerState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3 <---
	// volume3  | add | 4
	state = undoHandler.undo();
	EXPECT_EQ(3, undoHandler.statePosition());
	EXPECT_EQ(2, state.layer);
	EXPECT_EQ("Layer 3 Added", state.name);
	EXPECT_EQ(nullptr, state.volume);
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1 <---
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4
	state = undoHandler.undo();
	EXPECT_EQ(1, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_EQ(nullptr, state.volume);
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0 <---
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4
	state = undoHandler.undo();
	EXPECT_EQ(0, undoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(1, state.volume->region.getWidthInVoxels());
	EXPECT_FALSE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <---
	// null     | add | 3
	// volume3  | add | 4
	state = undoHandler.redo();
	EXPECT_EQ(2, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// null     | add | 3
	// volume3  | add | 4 <---
	state = undoHandler.redo();
	EXPECT_EQ(4, undoHandler.statePosition());
	EXPECT_EQ(2, state.layer);
	EXPECT_EQ("Layer 3 Added", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
	EXPECT_FALSE(undoHandler.canRedo());
}

TEST_F(MementoHandlerTest, testAddNewLayerEdit) {
	voxedit::MementoHandler undoHandler;
	ASSERT_TRUE(undoHandler.init());
	undoHandler.markUndo(0, "Layer 1", create(1));
	undoHandler.markLayerAdded(1, "Layer 2 Added", create(2));
	undoHandler.markUndo(1, "Layer 2 Modified", create(3));

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume3  | mod | 3 <---
	EXPECT_EQ(4, (int)undoHandler.stateSize());
	EXPECT_EQ(3, undoHandler.statePosition());

	LayerState state;

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <---
	// volume3  | mod | 3
	state = undoHandler.undo();
	EXPECT_EQ(2, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1 <---
	// volume2  | add | 2
	// volume3  | mod | 3
	state = undoHandler.undo();
	EXPECT_EQ(1, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	EXPECT_EQ(nullptr, state.volume);
	EXPECT_TRUE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0 <---
	// null     | add | 1
	// volume2  | add | 2
	// volume3  | mod | 3
	state = undoHandler.undo();
	EXPECT_EQ(0, undoHandler.statePosition());
	EXPECT_EQ(0, state.layer);
	EXPECT_EQ("Layer 1", state.name);
	EXPECT_NE(nullptr, state.volume);
	EXPECT_EQ(1, state.volume->region.getWidthInVoxels());
	EXPECT_FALSE(undoHandler.canUndo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2 <---
	// volume3  | mod | 3
	state = undoHandler.redo();
	EXPECT_EQ(2, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Added", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(2, state.volume->region.getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());

	// states:
	// ------------------
	// volume1  | mod | 0
	// null     | add | 1
	// volume2  | add | 2
	// volume3  | mod | 3 <---
	state = undoHandler.redo();
	EXPECT_EQ(3, undoHandler.statePosition());
	EXPECT_EQ(1, state.layer);
	EXPECT_EQ("Layer 2 Modified", state.name);
	ASSERT_NE(nullptr, state.volume);
	EXPECT_EQ(3, state.volume->region.getWidthInVoxels());
	EXPECT_FALSE(undoHandler.canRedo());
}

}
