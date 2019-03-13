/**
 * @file
 */

#include "../MementoHandler.h"

#include "core/tests/AbstractTest.h"
#include "voxel/polyvox/RawVolume.h"

namespace voxel {

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
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());

	undoHandler.markUndo(0, first);
	EXPECT_FALSE(undoHandler.canRedo()) << "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(undoHandler.canUndo()) << "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)undoHandler.stateSize());
	EXPECT_EQ(0, (int)undoHandler.statePosition());

	undoHandler.markUndo(0, second);
	EXPECT_FALSE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(2, (int)undoHandler.stateSize());
	EXPECT_EQ(1, (int)undoHandler.statePosition());

	undoHandler.markUndo(0, third);
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
	undoHandler.markUndo(0, first);
	undoHandler.markUndo(0, second);
	undoHandler.markUndo(0, third);

	std::pair<int, voxel::RawVolume*> undoThird = undoHandler.undo();
	ASSERT_NE(nullptr, undoThird.second);
	EXPECT_EQ(2, undoThird.second->region().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(1, (int)undoHandler.statePosition());

	std::pair<int, voxel::RawVolume*> undoSecond = undoHandler.undo();
	ASSERT_NE(nullptr, undoSecond.second);
	EXPECT_EQ(1, undoSecond.second->region().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());
	EXPECT_EQ(0, (int)undoHandler.statePosition());

	std::pair<int, voxel::RawVolume*> redoSecond = undoHandler.redo();
	ASSERT_NE(nullptr, redoSecond.second);
	EXPECT_EQ(2, redoSecond.second->region().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_TRUE(undoHandler.canUndo());
	EXPECT_EQ(1, (int)undoHandler.statePosition());

	undoSecond = undoHandler.undo();
	ASSERT_NE(nullptr, undoSecond.second);
	EXPECT_EQ(1, undoSecond.second->region().getWidthInVoxels());
	EXPECT_TRUE(undoHandler.canRedo());
	EXPECT_FALSE(undoHandler.canUndo());
	EXPECT_EQ(0, (int)undoHandler.statePosition());

	std::pair<int, voxel::RawVolume*> undoNotPossible = undoHandler.undo();
	ASSERT_EQ(nullptr, undoNotPossible.second);
}

}
