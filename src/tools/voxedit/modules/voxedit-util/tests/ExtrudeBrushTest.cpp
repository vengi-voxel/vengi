/**
 * @file
 */

#include "../modifier/brush/ExtrudeBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

// Direction convention:
//   positive depth  -> outward (along face normal, place voxels)
//   negative depth  -> inward  (opposite to face normal, erase voxels)
//
// Example: face = PositiveX (normal = +X), selected voxel at x=0
//   depth=+1 -> new voxel at x=+1  (outward)
//   depth=-1 -> voxel at x=-1 erased (inward / carve)
//
// FlagOutline is NOT transferred: selected voxels keep their flags unchanged,
// newly placed voxels use cursorVoxel as-is.

class ExtrudeBrushTest : public app::AbstractTest {
protected:
	static voxel::Voxel selectedVoxel(uint8_t color = 1) {
		voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, color);
		v.setFlags(voxel::FlagOutline);
		return v;
	}

	void executeExtrude(ExtrudeBrush &brush, scenegraph::SceneGraphNode &node, BrushContext &ctx,
						ModifierType modifierType = ModifierType::Place) {
		ctx.modifierType = modifierType;
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, modifierType);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
		brush.endBrush(ctx);
	}
};

// depth=-1, face=PositiveX -> carves inward: voxel at x=-1 is erased
TEST_F(ExtrudeBrushTest, testExtrudeCarveInward) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	// Solid block filling x=-3..0; surface voxel at x=0 is selected.
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = -3; x <= 0; ++x) {
		volume.setVoxel(x, 0, 0, x == 0 ? selectedVoxel() : solid);
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(-1);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// Inward from PositiveX face, depth=-1: selected voxel at x=0 is erased (step=0)
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()))
		<< "Selected voxel at (0,0,0) should be carved";
	// Next layer inward: untouched with depth=-1
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(-1, 0, 0).getMaterial()))
		<< "Voxel at (-1,0,0) should be untouched with depth=-1";
	// Outward side: nothing added
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 0, 0).getMaterial()))
		<< "No voxel should be placed outward at (1,0,0)";

	brush.shutdown();
}

// depth=+1, face=PositiveX -> new voxel placed outward at x=+1
TEST_F(ExtrudeBrushTest, testExtrudePlaceOutward) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(1);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// Outward from PositiveX face: x+1
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 0, 0).getMaterial()))
		<< "Voxel should be placed outward at (1,0,0)";
	// Inward side: nothing placed
	EXPECT_TRUE(voxel::isAir(volume.voxel(-1, 0, 0).getMaterial()))
		<< "No voxel should be placed inward at (-1,0,0)";

	brush.shutdown();
}

// depth=-2: two steps carved inward
TEST_F(ExtrudeBrushTest, testExtrudeCarveDepth2Inward) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = -5; x <= 0; ++x) {
		volume.setVoxel(x, 0, 0, x == 0 ? selectedVoxel() : solid);
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(-2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// depth=-2: step=0 erases (0,0,0), step=1 erases (-1,0,0)
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial())) << "Selected voxel at (0,0,0) should be carved";
	EXPECT_TRUE(voxel::isAir(volume.voxel(-1, 0, 0).getMaterial())) << "Voxel at (-1,0,0) should be carved";
	// Third layer untouched
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(-2, 0, 0).getMaterial())) << "Voxel at (-2,0,0) should be untouched";

	brush.shutdown();
}

// Non-selected voxels must not be carved or extruded
TEST_F(ExtrudeBrushTest, testExtrudeOnlySelected) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());
	volume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)); // no FlagOutline
	// Solid material behind the selection to carve
	volume.setVoxel(-1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(-1); // inward for PositiveX -> x=-1

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// depth=-1: selected voxel (0,0,0) is erased; (-1,0,0) is untouched
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()))
		<< "Selected voxel at (0,0,0) should be carved";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(-1, 0, 0).getMaterial()))
		<< "Voxel behind selection (-1,0,0) should be untouched with depth=-1";
	// Non-selected voxel at (0,1,0) must not be touched
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 1, 0).getMaterial()))
		<< "Non-selected voxel (0,1,0) itself must be untouched";

	brush.shutdown();
}

// After push inward (+), push outward (-) should restore carved voxel and add outward
TEST_F(ExtrudeBrushTest, testExtrudePushThenPull) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	// Solid block behind the selection so inward carve has material to remove
	volume.setVoxel(-1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	// Inward carve (-1): selected voxel (0,0,0) is erased
	brush.setDepth(-1);
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial())) << "Selected voxel at (0,0,0) should be carved";

	// Push outward (+1) from same selection: history restores (0,0,0), places at x=+1
	brush.setDepth(1);
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()))
		<< "Restored selected voxel at (0,0,0) after direction reversal";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 0, 0).getMaterial()))
		<< "Outward voxel at (1,0,0) expected";

	brush.shutdown();
}

// Fill sides: outward push (-) on 3x3 face should add side caps
TEST_F(ExtrudeBrushTest, testExtrudeFillSidesOutward) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			volume.setVoxel(x, y, 0, selectedVoxel());
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(1); // outward for PositiveZ -> z+1
	brush.setFillSides(true);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveZ;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// Outward slab at z=1
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 1).getMaterial())) << "Outward voxel at (0,0,1) expected";
	// Side caps adjacent to slab
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 0, 1).getMaterial())) << "Side cap at (2,0,1) expected";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(-2, 0, 1).getMaterial())) << "Side cap at (-2,0,1) expected";

	brush.shutdown();
}

// Without fill sides: no side caps
TEST_F(ExtrudeBrushTest, testExtrudeNoFillSides) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			volume.setVoxel(x, y, 0, selectedVoxel());
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(1); // outward
	brush.setFillSides(false);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveZ;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 1).getMaterial())) << "Outward voxel at (0,0,1) expected";
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 0, 1).getMaterial())) << "No side cap at (2,0,1) expected";

	brush.shutdown();
}

// beginBrush with no valid face succeeds but does not update the stored face
TEST_F(ExtrudeBrushTest, testBeginBrushNoFace) {
	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.targetVolumeRegion = voxel::Region(0, 5);
	ctx.cursorFace = voxel::FaceNames::Max;

	EXPECT_TRUE(brush.beginBrush(ctx)) << "beginBrush with no face should still succeed";
	EXPECT_EQ(brush.face(), voxel::FaceNames::Max) << "Face should remain Max when beginBrush called with Max";
	brush.endBrush(ctx);
	brush.shutdown();
}

// Double beginBrush without endBrush fails
TEST_F(ExtrudeBrushTest, testBeginBrushDoubleBegin) {
	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());

	BrushContext ctx;
	ctx.targetVolumeRegion = voxel::Region(0, 5);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_FALSE(brush.beginBrush(ctx)) << "Second beginBrush without endBrush should fail";
	brush.endBrush(ctx);
	brush.shutdown();
}

// endBrush resets active state but preserves depth and face (for panel usage)
TEST_F(ExtrudeBrushTest, testEndBrushResetsState) {
	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(3);

	BrushContext ctx;
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_TRUE(brush.active());
	brush.endBrush(ctx);

	EXPECT_FALSE(brush.active());
	EXPECT_EQ(brush.depth(), 3) << "Depth should be preserved across endBrush";
	EXPECT_EQ(brush.face(), voxel::FaceNames::PositiveX) << "Face should be preserved across endBrush for panel +/- usage";
	// beginBrush should work again after endBrush
	EXPECT_TRUE(brush.beginBrush(ctx));
	brush.endBrush(ctx);
	brush.shutdown();
}

} // namespace voxedit
