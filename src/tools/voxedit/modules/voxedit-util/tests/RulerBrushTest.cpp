/**
 * @file
 */

#include "voxedit-util/modifier/brush/RulerBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxel/Voxel.h"

namespace voxedit {

class RulerBrushTest : public app::AbstractTest {};

TEST_F(RulerBrushTest, testMeasurement) {
	RulerBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	// first click sets start
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);
	EXPECT_TRUE(brush.active());
	EXPECT_EQ(brush.startPos(), glm::ivec3(0, 0, 0));

	// move cursor - update tracks end position
	ctx.cursorPosition = glm::ivec3(3, 4, 0);
	brush.update(ctx, 0.0);

	// second click finalizes end
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);
	EXPECT_EQ(brush.endPos(), glm::ivec3(3, 4, 0));

	voxel::RawVolume volume(voxel::Region(-1, 5));
	// no voxels should be placed on the real volume
	EXPECT_FALSE(voxel::isBlocked(volume.voxel(glm::ivec3(1, 1, 0)).getMaterial()));

	EXPECT_EQ(brush.delta(), glm::ivec3(3, 4, 0));
	EXPECT_FLOAT_EQ(brush.euclideanDistance(), 5.0f);
	EXPECT_EQ(brush.manhattanDistance(), 7);

	brush.shutdown();
}

TEST_F(RulerBrushTest, testPreviewDoesNotModifyRealVolume) {
	RulerBrush brush;
	ASSERT_TRUE(brush.init());

	voxel::RawVolume realVolume(voxel::Region(0, 10));

	scenegraph::SceneGraphNode realNode(scenegraph::SceneGraphNodeType::Model);
	realNode.setUnownedVolume(&realVolume);

	scenegraph::SceneGraph sceneGraph;

	BrushContext ctx;
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	// first click sets start
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);

	// move cursor - update tracks end
	ctx.cursorPosition = glm::ivec3(5, 5, 5);
	brush.update(ctx, 0.0);

	// between clicks: gizmo shows tracking line
	EXPECT_TRUE(brush.wantBrushGizmo(ctx));
	BrushGizmoState state;
	brush.brushGizmoState(ctx, state);
	EXPECT_EQ(state.operations, (uint32_t)BrushGizmo_Line);
	EXPECT_EQ(state.numPositions, 2);
	EXPECT_EQ(state.positions[0], glm::vec3(0.5f));
	EXPECT_EQ(state.positions[1], glm::vec3(5.5f));

	// second click finalizes end
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);

	// no voxels placed
	EXPECT_FALSE(voxel::isBlocked(realVolume.voxel(glm::ivec3(2, 2, 2)).getMaterial()));

	brush.shutdown();
}

TEST_F(RulerBrushTest, testDeltaIsAbsolute) {
	RulerBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.cursorPosition = glm::ivec3(5, 5, 5);
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	// first click sets start
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);

	// move cursor and finalize with second click
	ctx.cursorPosition = glm::ivec3(2, 1, 3);
	brush.update(ctx, 0.0);
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);

	// delta should be absolute values even when end < start
	EXPECT_EQ(brush.delta(), glm::ivec3(3, 4, 2));

	brush.shutdown();
}

TEST_F(RulerBrushTest, testMeasurementPersistsAfterEndBrush) {
	RulerBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	// first click sets start
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);

	// move cursor, then second click sets end
	ctx.cursorPosition = glm::ivec3(5, 5, 5);
	brush.update(ctx, 0.0);
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);

	// measurement should persist after both clicks
	EXPECT_TRUE(brush.active());
	EXPECT_TRUE(brush.hasMeasurement());
	EXPECT_TRUE(brush.wantBrushGizmo(ctx));
	EXPECT_EQ(brush.startPos(), glm::ivec3(0, 0, 0));
	EXPECT_EQ(brush.endPos(), glm::ivec3(5, 5, 5));

	// cursor movement should NOT update the end position after measurement is complete
	ctx.cursorPosition = glm::ivec3(9, 9, 9);
	brush.update(ctx, 0.0);
	EXPECT_EQ(brush.endPos(), glm::ivec3(5, 5, 5));

	brush.shutdown();
}

TEST_F(RulerBrushTest, testReferencePositionMode) {
	RulerBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.referencePos = glm::ivec3(1, 2, 3);
	ctx.cursorPosition = glm::ivec3(4, 6, 3);
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	brush.setUseReferencePos(true);
	EXPECT_TRUE(brush.useReferencePos());

	// no measurement before clicking
	EXPECT_FALSE(brush.active());

	// click sets end position and activates measurement
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);
	EXPECT_TRUE(brush.active());
	EXPECT_EQ(brush.startPos(), glm::ivec3(1, 2, 3));
	EXPECT_EQ(brush.endPos(), glm::ivec3(4, 6, 3));
	EXPECT_FLOAT_EQ(brush.euclideanDistance(), 5.0f);

	// cursor movement should NOT update end position (persisted)
	ctx.cursorPosition = glm::ivec3(9, 9, 9);
	brush.update(ctx, 0.0);
	EXPECT_EQ(brush.endPos(), glm::ivec3(4, 6, 3));

	// but start should track reference position changes
	ctx.referencePos = glm::ivec3(2, 3, 4);
	brush.update(ctx, 0.0);
	EXPECT_EQ(brush.startPos(), glm::ivec3(2, 3, 4));

	// gizmo should show the line
	EXPECT_TRUE(brush.wantBrushGizmo(ctx));
	BrushGizmoState state;
	brush.brushGizmoState(ctx, state);
	EXPECT_EQ(state.operations, (uint32_t)BrushGizmo_Line);
	EXPECT_EQ(state.numPositions, 2);

	// next click updates end position
	ctx.cursorPosition = glm::ivec3(7, 8, 9);
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);
	EXPECT_EQ(brush.endPos(), glm::ivec3(7, 8, 9));

	// switching off reference mode resets state
	brush.setUseReferencePos(false);
	EXPECT_FALSE(brush.active());

	brush.shutdown();
}

} // namespace voxedit
