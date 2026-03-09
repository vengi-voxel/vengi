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
// FlagOutline handling:
//   - Original selected voxels have FlagOutline cleared during extrusion.
//   - Only the outermost (tip) layer gets FlagOutline for chaining extrudes.
//   - On carve, the voxel behind the deepest carved layer gets FlagOutline.

class ExtrudeBrushTest : public app::AbstractTest {
protected:
	static voxel::Voxel selectedVoxel(uint8_t color = 1) {
		voxel::Voxel vx = voxel::createVoxel(voxel::VoxelType::Generic, color);
		vx.setFlags(voxel::FlagOutline);
		return vx;
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

// Fill sides: extrude next to an existing wall fills the gap between extrusion and wall
TEST_F(ExtrudeBrushTest, testExtrudeFillSidesWithWall) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	// Selected voxel at (0,0,0) recessed inside a box. Wall neighbor at (1,0,0) extends
	// upward to (1,0,3). Extruding by 2 should fill the gap at (1,0,1) and (1,0,2).
	volume.setVoxel(0, 0, 0, selectedVoxel());
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	// Wall column next to the selection, extending upward
	for (int z = 0; z <= 3; ++z) {
		volume.setVoxel(2, 0, z, solid);
	}
	// Neighbor at (1,0,0) with solid above it at (1,0,1) triggers fill-sides
	volume.setVoxel(1, 0, 0, solid);
	volume.setVoxel(1, 0, 1, solid);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(2); // outward for PositiveZ -> z+1, z+2

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveZ;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// Extruded voxels at z=1 and z=2
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 1).getMaterial())) << "Extruded voxel at (0,0,1) expected";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 2).getMaterial())) << "Tip voxel at (0,0,2) expected";
	// Side wall fills gap: (1,0,2) was air, should now be filled by fill-sides
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 0, 2).getMaterial()))
		<< "Side wall at (1,0,2) should fill gap next to existing wall";

	brush.shutdown();
}

// No fill sides on flat surface: single voxel extrusion should not create rim
TEST_F(ExtrudeBrushTest, testExtrudeNoRimOnFlatSurface) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	// 3x3 flat surface at z=0, center voxel selected
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			volume.setVoxel(x, y, 0, (x == 0 && y == 0) ? selectedVoxel() : solid);
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(1); // outward for PositiveZ -> z+1

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveZ;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// Only 1 extruded voxel expected, no rim
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 1).getMaterial())) << "Extruded voxel at (0,0,1) expected";
	EXPECT_TRUE(voxel::isAir(volume.voxel(1, 0, 1).getMaterial())) << "No rim voxel at (1,0,1)";
	EXPECT_TRUE(voxel::isAir(volume.voxel(-1, 0, 1).getMaterial())) << "No rim voxel at (-1,0,1)";
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 1, 1).getMaterial())) << "No rim voxel at (0,1,1)";
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, -1, 1).getMaterial())) << "No rim voxel at (0,-1,1)";

	brush.shutdown();
}

// Lateral offset: depth=2 with offsetU=2 on PositiveZ face shifts along first perpendicular axis
TEST_F(ExtrudeBrushTest, testExtrudeWithOffsetU) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(2);
	brush.setOffsetU(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveZ;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// PositiveZ face: axis=Z(2), perp1=(2+1)%3=X(0), perp2=(2+2)%3=Y(1)
	// offsetU=2 along X: step 1 shift = int(1.0*1)=1, step 2 shift = int(1.0*2)=2
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(1, 0, 1).getMaterial()))
		<< "Step 1 voxel at (1,0,1) expected";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(2, 0, 2).getMaterial()))
		<< "Step 2 (tip) voxel at (2,0,2) expected";
	// Original position should not have a new voxel at z=1 without offset
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 1).getMaterial()))
		<< "No voxel at (0,0,1) - offset shifts it to (1,0,1)";

	brush.shutdown();
}

// Tip-only selection: only outermost extruded layer gets FlagOutline
TEST_F(ExtrudeBrushTest, testExtrudeTipOnlySelection) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	ExtrudeBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setDepth(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeExtrude(brush, node, ctx);

	// Original voxel should have FlagOutline cleared
	const voxel::Voxel origVoxel = volume.voxel(0, 0, 0);
	EXPECT_FALSE(origVoxel.getFlags() & voxel::FlagOutline)
		<< "Original voxel at (0,0,0) should not have FlagOutline after extrusion";
	// Step 1 (intermediate) should NOT have FlagOutline
	const voxel::Voxel midVoxel = volume.voxel(1, 0, 0);
	EXPECT_TRUE(voxel::isBlocked(midVoxel.getMaterial()));
	EXPECT_FALSE(midVoxel.getFlags() & voxel::FlagOutline)
		<< "Intermediate voxel at (1,0,0) should not have FlagOutline";
	// Step 2 (tip) should have FlagOutline
	const voxel::Voxel tipVoxel = volume.voxel(2, 0, 0);
	EXPECT_TRUE(voxel::isBlocked(tipVoxel.getMaterial()));
	EXPECT_TRUE(tipVoxel.getFlags() & voxel::FlagOutline)
		<< "Tip voxel at (2,0,0) should have FlagOutline for chaining";

	brush.shutdown();
}

// Carve-behind selection: after carving, the voxel behind the deepest carved layer gets FlagOutline
TEST_F(ExtrudeBrushTest, testExtrudeCarveBehindSelection) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	// Solid block from x=-3 to x=0, surface at x=0 is selected
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

	// Carved voxel at x=0 should be air
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()));
	// Behind the carved layer at x=-1 should now have FlagOutline
	const voxel::Voxel behindVoxel = volume.voxel(-1, 0, 0);
	EXPECT_TRUE(voxel::isBlocked(behindVoxel.getMaterial()));
	EXPECT_TRUE(behindVoxel.getFlags() & voxel::FlagOutline)
		<< "Voxel behind carved layer at (-1,0,0) should have FlagOutline for selection continuity";

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
