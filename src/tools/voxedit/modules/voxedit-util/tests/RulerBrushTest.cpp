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

	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_TRUE(brush.active());
	EXPECT_EQ(brush.startPos(), glm::ivec3(0, 0, 0));

	ctx.cursorPosition = glm::ivec3(3, 4, 0);
	voxel::RawVolume volume(voxel::Region(-1, 5));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());

	brush.preExecute(ctx, &volume);
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, ctx));
	EXPECT_EQ(brush.endPos(), glm::ivec3(3, 4, 0));

	// no voxels should be placed on the real volume
	EXPECT_FALSE(voxel::isBlocked(volume.voxel(glm::ivec3(1, 1, 0)).getMaterial()));

	EXPECT_EQ(brush.delta(), glm::ivec3(3, 4, 0));
	EXPECT_FLOAT_EQ(brush.euclideanDistance(), 5.0f);
	EXPECT_EQ(brush.manhattanDistance(), 7);

	brush.endBrush(ctx);
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

	EXPECT_TRUE(brush.beginBrush(ctx));

	ctx.cursorPosition = glm::ivec3(5, 5, 5);

	// execute should not place any voxels
	brush.preExecute(ctx, &realVolume);
	ModifierVolumeWrapper realWrapper(realNode, brush.modifierType());
	EXPECT_TRUE(brush.execute(sceneGraph, realWrapper, ctx));
	EXPECT_FALSE(voxel::isBlocked(realVolume.voxel(glm::ivec3(2, 2, 2)).getMaterial()));

	// the brush should provide a line gizmo instead
	EXPECT_TRUE(brush.wantBrushGizmo(ctx));
	BrushGizmoState state;
	brush.brushGizmoState(ctx, state);
	EXPECT_EQ(state.operations, (uint32_t)BrushGizmo_Line);
	EXPECT_EQ(state.numPositions, 2);
	EXPECT_EQ(state.positions[0], glm::vec3(0.5f));
	EXPECT_EQ(state.positions[1], glm::vec3(5.5f));

	brush.endBrush(ctx);
	brush.shutdown();
}

TEST_F(RulerBrushTest, testDeltaIsAbsolute) {
	RulerBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.cursorPosition = glm::ivec3(5, 5, 5);
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	EXPECT_TRUE(brush.beginBrush(ctx));

	ctx.cursorPosition = glm::ivec3(2, 1, 3);
	voxel::RawVolume volume(voxel::Region(0, 10));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, brush.modifierType());

	brush.preExecute(ctx, &volume);
	brush.execute(sceneGraph, wrapper, ctx);

	// delta should be absolute values even when end < start
	EXPECT_EQ(brush.delta(), glm::ivec3(3, 4, 2));

	brush.endBrush(ctx);
	brush.shutdown();
}

} // namespace voxedit
