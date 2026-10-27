/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/BoxGesture.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StrokeGesture.h"

namespace voxedit {

class BrushGestureTest : public app::AbstractTest {};

TEST_F(BrushGestureTest, testBoxCapturesPlaneBeforeDepth) {
	BoxGesture gesture;
	BrushContext ctx;
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(1, 2, 3);
	gesture.begin(ctx, true);
	ctx.cursorPosition = glm::ivec3(4, 2, 7);
	EXPECT_TRUE(BoxGesture::needsDepth(ctx, gesture.region(ctx, false)));
	ASSERT_TRUE(gesture.step(ctx, 0));
	ctx.cursorPosition = glm::ivec3(20, 6, 30);
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	EXPECT_EQ(gesture.cursorPosition(ctx, 0), glm::ivec3(4, 6, 7));
	EXPECT_EQ(gesture.region(ctx, false), voxel::Region(1, 2, 3, 4, 6, 7));
	EXPECT_FALSE(BoxGesture::needsDepth(ctx, gesture.region(ctx, false)));
	gesture.end();
	EXPECT_FALSE(gesture.active());
	EXPECT_FALSE(gesture.hasSecondPosition());
	EXPECT_EQ(gesture.cursorPosition(ctx, 0), ctx.cursorPosition);
}

TEST_F(BrushGestureTest, testBoxCenterAndGrid) {
	BoxGesture gesture;
	BrushContext ctx;
	ctx.gridResolution = glm::ivec3(2, 3, 4);
	ctx.cursorPosition = glm::ivec3(5, 7, 11);
	gesture.begin(ctx, true);
	EXPECT_EQ(gesture.firstPosition(), glm::ivec3(4, 6, 8));
	ctx.cursorPosition = glm::ivec3(2, 3, 4);
	EXPECT_EQ(gesture.region(ctx, false), voxel::Region(2, 3, 4, 5, 8, 11));
	EXPECT_EQ(gesture.region(ctx, true), voxel::Region(2, 3, 4, 6, 9, 12));
	EXPECT_EQ(BoxGesture::snapToGrid(glm::ivec3(-5, -7, -11), ctx.gridResolution), glm::ivec3(-4, -6, -8));
}

TEST_F(BrushGestureTest, testBoxAxisLockAndPlaneBounds) {
	BoxGesture gesture;
	BrushContext ctx;
	ctx.cursorFace = voxel::FaceNames::NegativeZ;
	gesture.begin(ctx, true);
	ctx.cursorPosition = glm::ivec3(4, 5, 0);
	ctx.lockedAxis = math::Axis::Z;
	EXPECT_FALSE(BoxGesture::needsDepth(ctx, gesture.region(ctx, false)));
	EXPECT_FALSE(gesture.step(ctx, 0));
	EXPECT_FALSE(gesture.hasSecondPosition());
	ctx.lockedAxis = math::Axis::None;
	EXPECT_FALSE(gesture.step(ctx, 2));
	gesture.setPlaneBounds(voxel::Region(1, 2, 3, 4, 5, 3));
	ctx.cursorPosition = glm::ivec3(99, 99, -2);
	EXPECT_EQ(gesture.region(ctx, false), voxel::Region(1, 2, -2, 4, 5, 3));
	EXPECT_EQ(gesture.cursorPosition(ctx, 2), glm::ivec3(4, 5, 3));
	gesture.reset();
	EXPECT_FALSE(gesture.active());
	EXPECT_FALSE(gesture.hasSecondPosition());
	EXPECT_EQ(gesture.face(), voxel::FaceNames::Max);
	EXPECT_EQ(gesture.firstPosition(), glm::ivec3(0));
}

TEST_F(BrushGestureTest, testStrokeFillsGapsAndRepeatsStationarySamples) {
	StrokeGesture gesture;
	core::DynamicArray<glm::ivec3> positions;
	auto visit = [&](const glm::ivec3 &pos) { positions.push_back(pos); };
	gesture.begin();
	gesture.visit(glm::ivec3(0), visit);
	gesture.visit(glm::ivec3(3, 0, 0), visit);
	ASSERT_EQ(positions.size(), 4u);
	for (int i = 0; i < 4; ++i) {
		EXPECT_EQ(positions[i], glm::ivec3(i, 0, 0));
	}
	gesture.visit(glm::ivec3(3, 0, 0), visit);
	ASSERT_EQ(positions.size(), 5u);
	EXPECT_EQ(positions.back(), glm::ivec3(3, 0, 0));
	gesture.end();
	gesture.begin();
	positions.clear();
	gesture.visit(glm::ivec3(10), visit);
	ASSERT_EQ(positions.size(), 1u) << "A new stroke must not connect to the previous path";
	EXPECT_EQ(positions[0], glm::ivec3(10));
}

TEST_F(BrushGestureTest, testStrokeDefersUndoUntilRelease) {
	StrokeGesture gesture;
	gesture.begin();
	gesture.accumulate(voxel::Region(1, 2, 3, 2, 3, 4));
	gesture.accumulate(voxel::Region::InvalidRegion);
	gesture.accumulate(voxel::Region(-1, 0, 2, 1, 2, 3));
	EXPECT_FALSE(gesture.consumePendingUndoRegion().isValid());
	gesture.end();
	EXPECT_FALSE(gesture.active());
	EXPECT_EQ(gesture.consumePendingUndoRegion(), voxel::Region(-1, 0, 2, 2, 3, 4));
	EXPECT_FALSE(gesture.consumePendingUndoRegion().isValid());
	gesture.begin();
	gesture.accumulate(voxel::Region(0, 2));
	gesture.reset();
	gesture.end();
	EXPECT_FALSE(gesture.consumePendingUndoRegion().isValid());
}

TEST_F(BrushGestureTest, testStrokeFootprint) {
	const glm::ivec3 position(4, 5, 6);
	EXPECT_EQ(StrokeGesture::region(position, 0, glm::ivec3(1, 2, 3)), voxel::Region(4, 5, 6, 4, 6, 8));
	EXPECT_EQ(StrokeGesture::region(position, 2, glm::ivec3(1, 2, 3)), voxel::Region(2, 3, 4, 6, 7, 8));
}

TEST_F(BrushGestureTest, testPreviewDoesNotAdvanceShapeStroke) {
	ShapeBrush brush;
	ASSERT_TRUE(brush.init());
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::RawVolume volume(voxel::Region(0, 0, 0, 12, 1, 1));
	node.setUnownedVolume(&volume);
	BrushContext ctx;
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	ASSERT_TRUE(brush.beginBrush(ctx));
	ModifierVolumeWrapper first(node, ModifierType::Place);
	ASSERT_TRUE(brush.execute(sceneGraph, first, ctx));

	voxel::RawVolume previewVolume(volume.region());
	scenegraph::SceneGraphNode previewNode(scenegraph::SceneGraphNodeType::Model);
	previewNode.setUnownedVolume(&previewVolume);
	ModifierVolumeWrapper preview(previewNode, ModifierType::Place);
	ctx.cursorPosition.x = 10;
	ctx.preview = true;
	ASSERT_TRUE(brush.execute(sceneGraph, preview, ctx));

	ctx.cursorPosition.x = 3;
	ctx.preview = false;
	ModifierVolumeWrapper last(node, ModifierType::Place);
	ASSERT_TRUE(brush.execute(sceneGraph, last, ctx));
	brush.endBrush(ctx);
	for (int x = 0; x <= 3; ++x) {
		EXPECT_FALSE(voxel::isAir(volume.voxel(x, 0, 0).getMaterial())) << x;
	}
	for (int x = 4; x <= 10; ++x) {
		EXPECT_TRUE(voxel::isAir(volume.voxel(x, 0, 0).getMaterial())) << x;
	}
	EXPECT_EQ(brush.consumePendingUndoRegion(), voxel::Region(0, 0, 0, 3, 0, 0));
	EXPECT_FALSE(brush.consumePendingUndoRegion().isValid());
	brush.shutdown();
}

TEST_F(BrushGestureTest, testBrushResetPreservesSettingsButDiscardsGesture) {
	ShapeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setCenterMode();
	brush.setRadius(3);
	BrushContext ctx;
	ctx.cursorFace = voxel::FaceNames::PositiveZ;
	ASSERT_TRUE(brush.beginBrush(ctx));
	ctx.cursorPosition = glm::ivec3(4, 5, 0);
	brush.step(ctx);
	brush.reset();
	EXPECT_TRUE(brush.centerMode());
	EXPECT_EQ(brush.currentCursorPosition(ctx), ctx.cursorPosition);
	ASSERT_TRUE(brush.beginBrush(ctx));
	brush.setStrokeMode();
	ASSERT_TRUE(brush.beginBrush(ctx)) << "Stroke must replace an unfinished box gesture";
	EXPECT_EQ(brush.radius(), 3);
	EXPECT_TRUE(brush.strokeActive());
	brush.onSceneChange();
	EXPECT_FALSE(brush.strokeActive());
	EXPECT_FALSE(brush.consumePendingUndoRegion().isValid());
	EXPECT_TRUE(brush.strokeMode());
	EXPECT_EQ(brush.radius(), 3);
	brush.shutdown();
}

} // namespace voxedit
