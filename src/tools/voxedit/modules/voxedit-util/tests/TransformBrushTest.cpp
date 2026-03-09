/**
 * @file
 */

#include "../modifier/brush/TransformBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class TransformBrushTest : public app::AbstractTest {
protected:
	static voxel::Voxel selectedVoxel(uint8_t color = 1) {
		voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
		voxel.setFlags(voxel::FlagOutline);
		return voxel;
	}

	void executeTransform(TransformBrush &brush, scenegraph::SceneGraphNode &node, BrushContext &ctx) {
		ctx.modifierType = ModifierType::Override;
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, ModifierType::Override);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
		brush.endBrush(ctx);
	}
};

TEST_F(TransformBrushTest, testMovePositiveX) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(2, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);

	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()))
		<< "Original position should be empty after move";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 0, 0).getMaterial()))
		<< "Voxel should be at new position (2,0,0)";
	EXPECT_TRUE(volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline)
		<< "Moved voxel should retain selection flag";

	brush.shutdown();
}

TEST_F(TransformBrushTest, testMoveReversible) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// Move to +3
	brush.setMoveOffset(glm::ivec3(3, 0, 0));
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()));

	// Move back to 0 (same snapshot, different offset)
	brush.setMoveOffset(glm::ivec3(0, 0, 0));
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()))
		<< "Voxel should be back at original position";
	EXPECT_TRUE(voxel::isAir(volume.voxel(3, 0, 0).getMaterial()))
		<< "Previous moved position should be empty";

	brush.shutdown();
}

TEST_F(TransformBrushTest, testScaleUp) {
	voxel::RawVolume volume(voxel::Region(-10, 10));
	// Create a 3x3x1 selected slab at y=0, z=0
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			volume.setVoxel(x, y, 0, selectedVoxel());
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Scale);
	brush.setScale(glm::vec3(2.0f, 2.0f, 1.0f));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);

	// Scaled up 2x: center should still have a voxel, and edges should be further out
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()))
		<< "Center should still be filled";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 0, 0).getMaterial()))
		<< "Scaled edge should be filled (no gaps)";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 0, 0).getMaterial()))
		<< "Intermediate position should be filled (no gaps)";

	brush.shutdown();
}

TEST_F(TransformBrushTest, testRotate90) {
	voxel::RawVolume volume(voxel::Region(-10, 10));
	// Horizontal line along X at y=0, z=0
	for (int x = -2; x <= 2; ++x) {
		volume.setVoxel(x, 0, 0, selectedVoxel());
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Rotate);
	brush.setRotationDegrees(glm::vec3(0.0f, 0.0f, 90.0f));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);

	// After 90-degree Z rotation, line along X should become line along Y
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 2, 0).getMaterial()))
		<< "Rotated voxel should be at (0,2,0)";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, -2, 0).getMaterial()))
		<< "Rotated voxel should be at (0,-2,0)";
	// Original X positions should be empty (except center which maps to center)
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 0, 0).getMaterial()))
		<< "Original position (2,0,0) should be empty after rotation";

	brush.shutdown();
}

TEST_F(TransformBrushTest, testDoubleBeginFails) {
	TransformBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.targetVolumeRegion = voxel::Region(0, 5);

	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_FALSE(brush.beginBrush(ctx)) << "Second beginBrush without endBrush should fail";
	brush.endBrush(ctx);
	brush.shutdown();
}

TEST_F(TransformBrushTest, testCommitOnModeSwitch) {
	voxel::RawVolume volume(voxel::Region(-10, 10));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(3, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// Execute move
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()));

	// Switch to scale: should commit the move (not jump back)
	brush.setTransformMode(TransformMode::Scale);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()))
		<< "Moved voxel should stay at (3,0,0) after mode switch";
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()))
		<< "Original position should remain empty after mode switch";

	brush.shutdown();
}

} // namespace voxedit
