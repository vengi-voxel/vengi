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
	}
};

TEST_F(TransformBrushTest, testMovePositiveX) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(2, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);

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
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// Move to +3
	brush.setMoveOffset(glm::ivec3(3, 0, 0));
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()));

	// Move back to 0 (same snapshot, different offset)
	brush.setMoveOffset(glm::ivec3(0, 0, 0));
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);
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
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Scale);
	brush.setScale(glm::vec3(2.0f, 2.0f, 1.0f));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);

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
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Rotate);
	brush.setRotationDegrees(glm::vec3(0.0f, 0.0f, 90.0f));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);

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
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(3, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// Execute move
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()));

	// Switch to scale: should commit the move (not jump back)
	brush.setTransformMode(TransformMode::Scale);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()))
		<< "Moved voxel should stay at (3,0,0) after mode switch";
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()))
		<< "Original position should remain empty after mode switch";

	// Now execute scale from the new position (3,0,0)
	brush.setScale(glm::vec3(1.0f));
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()))
		<< "Identity scale should keep voxel at (3,0,0)";

	brush.shutdown();
}

// onDeactivated returns true when snapshot exists
TEST_F(TransformBrushTest, testOnDeactivatedWithSnapshot) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(1, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// Before execute, no snapshot
	EXPECT_FALSE(brush.onDeactivated()) << "No snapshot yet";

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);

	// After execute, snapshot exists
	EXPECT_TRUE(brush.hasSnapshot());
	EXPECT_TRUE(brush.onDeactivated()) << "Should commit pending transform";

	brush.shutdown();
}

// onDeactivated returns false when no snapshot
TEST_F(TransformBrushTest, testOnDeactivatedWithoutSnapshot) {
	TransformBrush brush;
	ASSERT_TRUE(brush.init());

	EXPECT_FALSE(brush.onDeactivated()) << "No snapshot - nothing to commit";

	brush.shutdown();
}

// onActivated resets the brush
TEST_F(TransformBrushTest, testOnActivatedResetsState) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(3, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);
	EXPECT_TRUE(brush.hasSnapshot());

	brush.onActivated();
	EXPECT_FALSE(brush.hasSnapshot()) << "Snapshot should be cleared by onActivated";
	EXPECT_EQ(brush.moveOffset(), glm::ivec3(0)) << "Move offset should be reset";

	brush.shutdown();
}

// Move preserves all voxels
TEST_F(TransformBrushTest, testMovePreservesSurfaceVoxels) {
	voxel::RawVolume volume(voxel::Region(-10, 10));
	// Create a 3x3x3 cube of selected voxels
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			for (int z = -1; z <= 1; ++z) {
				volume.setVoxel(x, y, z, selectedVoxel());
			}
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(5, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);

	// Count voxels at new position - all 27 should be there
	int count = 0;
	for (int x = 4; x <= 6; ++x) {
		for (int y = -1; y <= 1; ++y) {
			for (int z = -1; z <= 1; ++z) {
				if (voxel::isBlocked(volume.voxel(x, y, z).getMaterial())) {
					++count;
				}
			}
		}
	}
	EXPECT_EQ(count, 27) << "All 27 voxels should be preserved after move";

	// Original positions should be empty
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			for (int z = -1; z <= 1; ++z) {
				EXPECT_TRUE(voxel::isAir(volume.voxel(x, y, z).getMaterial()))
					<< "Original position (" << x << "," << y << "," << z << ") should be empty";
			}
		}
	}

	brush.shutdown();
}

// Non-selected voxels must not be moved
TEST_F(TransformBrushTest, testMoveDoesNotAffectNonSelectedVoxels) {
	voxel::RawVolume volume(voxel::Region(-10, 10));
	// Place a selected voxel at (0,0,0)
	volume.setVoxel(0, 0, 0, selectedVoxel());
	// Place a non-selected voxel at (1,0,0) - no FlagOutline
	volume.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Move);
	brush.setMoveOffset(glm::ivec3(5, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);

	// Selected voxel should have moved to (5,0,0)
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(5, 0, 0).getMaterial()))
		<< "Selected voxel should be at new position (5,0,0)";
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()))
		<< "Original selected position should be empty";

	// Non-selected voxel should remain at (1,0,0)
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 0, 0).getMaterial()))
		<< "Non-selected voxel should remain at (1,0,0)";
	EXPECT_EQ(volume.voxel(1, 0, 0).getColor(), 2)
		<< "Non-selected voxel should retain its color";
	EXPECT_FALSE(volume.voxel(1, 0, 0).getFlags() & voxel::FlagOutline)
		<< "Non-selected voxel should still have no selection flag";

	brush.shutdown();
}

// Shear preserves all voxels
TEST_F(TransformBrushTest, testShearPreservesAllVoxels) {
	voxel::RawVolume volume(voxel::Region(-10, 10));
	// Vertical column along Y so shear X (which shifts by Y-layer) has effect.
	// Center = (0, 0, 0). shearOffset.x=4 shifts each Y-layer by (relative.y/halfSize.y)*4.
	// Voxel at y=+2: relative.y=2, halfSize.y=2.5 -> shift.x = (2/2.5)*4 = 3.2 -> 3
	// Voxel at y=-2: shift.x = (-2/2.5)*4 = -3.2 -> -3
	for (int y = -2; y <= 2; ++y) {
		volume.setVoxel(0, y, 0, selectedVoxel());
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	TransformBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setTransformMode(TransformMode::Shear);
	brush.setShearOffset(glm::ivec3(4, 0, 0));

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeTransform(brush, node, ctx);
	brush.endBrush(ctx);

	// Top voxel (y=+2) should have shifted in +X
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 2, 0).getMaterial()))
		<< "Top voxel should be sheared to x=+3";
	// Bottom voxel (y=-2) should have shifted in -X
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(-3, -2, 0).getMaterial()))
		<< "Bottom voxel should be sheared to x=-3";
	// Center voxel (y=0) should stay at x=0 (relative.y=0)
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()))
		<< "Center voxel should remain at origin";

	brush.shutdown();
}

} // namespace voxedit
