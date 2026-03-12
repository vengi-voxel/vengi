/**
 * @file
 */

#include "../modifier/brush/SculptBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class SculptBrushTest : public app::AbstractTest {
protected:
	static voxel::Voxel selectedVoxel(uint8_t color = 1) {
		voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
		voxel.setFlags(voxel::FlagOutline);
		return voxel;
	}

	void executeSculpt(SculptBrush &brush, scenegraph::SceneGraphNode &node, BrushContext &ctx) {
		ctx.modifierType = ModifierType::Override;
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, ModifierType::Override);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
	}
};

TEST_F(SculptBrushTest, testErodeRemovesProtrusion) {
	// A single voxel sticking out of a flat 3x3 surface should be removed
	voxel::RawVolume volume(voxel::Region(-5, 5));

	// Create a 3x3 flat surface at y=0
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			volume.setVoxel(x, 0, z, selectedVoxel());
		}
	}
	// Add a protrusion at (0,1,0)
	volume.setVoxel(0, 1, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SculptBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSculptMode(SculptMode::Erode);
	// strength=0.5 -> removeThreshold=2: removes voxels with 0-1 solid face-neighbors
	brush.setStrength(0.5f);
	brush.setIterations(1);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeSculpt(brush, node, ctx);
	brush.endBrush(ctx);

	// The protrusion has 1 solid face-neighbor (below) -> 1 < 2 -> removed
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 1, 0).getMaterial()))
		<< "Protrusion at (0,1,0) should be eroded";

	// The flat surface should remain (corner=2, edge=3, center=4 neighbors, all >= 2)
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 0, z).getMaterial()))
				<< "Surface voxel at (" << x << ",0," << z << ") should remain";
		}
	}

	brush.shutdown();
}

TEST_F(SculptBrushTest, testErodePreservesFlatSurface) {
	// A flat 3x3 surface should survive moderate erosion.
	// Corner voxels have 2 face-neighbors, edge=3, center=4.
	// strength=0.4 -> threshold=2, so all voxels have >= 2 neighbors and survive.
	voxel::RawVolume volume(voxel::Region(-5, 5));

	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			volume.setVoxel(x, 0, z, selectedVoxel());
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SculptBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSculptMode(SculptMode::Erode);
	brush.setStrength(0.4f);
	brush.setIterations(10);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeSculpt(brush, node, ctx);
	brush.endBrush(ctx);

	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 0, z).getMaterial()))
				<< "Flat surface voxel at (" << x << ",0," << z << ") should not be eroded";
		}
	}

	brush.shutdown();
}

TEST_F(SculptBrushTest, testErodeReversible) {
	// Changing strength should re-apply from snapshot, not accumulate
	voxel::RawVolume volume(voxel::Region(-5, 5));

	// 3x3 surface with protrusion
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			volume.setVoxel(x, 0, z, selectedVoxel());
		}
	}
	volume.setVoxel(0, 1, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SculptBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSculptMode(SculptMode::Erode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// First pass: moderate strength removes protrusion (threshold=2, protrusion has 1 neighbor)
	brush.setStrength(0.5f);
	ASSERT_TRUE(brush.beginBrush(ctx));
	executeSculpt(brush, node, ctx);

	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 1, 0).getMaterial()))
		<< "Protrusion should be removed at high strength";

	// Second pass: zero strength should restore protrusion (re-applied from snapshot)
	brush.setStrength(0.0f);
	executeSculpt(brush, node, ctx);

	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 1, 0).getMaterial()))
		<< "Protrusion should be restored when strength=0 (no-op from snapshot)";

	brush.endBrush(ctx);
	brush.shutdown();
}

TEST_F(SculptBrushTest, testErodePreservesUnselected) {
	// Non-selected voxels should not be modified
	voxel::RawVolume volume(voxel::Region(-5, 5));

	// Selected surface
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			volume.setVoxel(x, 0, z, selectedVoxel());
		}
	}
	volume.setVoxel(0, 1, 0, selectedVoxel());

	// Non-selected voxel nearby
	voxel::Voxel unselected = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	volume.setVoxel(3, 0, 0, unselected);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SculptBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSculptMode(SculptMode::Erode);
	brush.setStrength(1.0f);
	brush.setIterations(3);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeSculpt(brush, node, ctx);
	brush.endBrush(ctx);

	// Non-selected voxel should remain unchanged
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(3, 0, 0).getMaterial()))
		<< "Non-selected voxel should not be removed";
	EXPECT_EQ(volume.voxel(3, 0, 0).getColor(), 3)
		<< "Non-selected voxel color should be preserved";
	EXPECT_FALSE((volume.voxel(3, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Non-selected voxel should not gain selection flag";

	brush.shutdown();
}

TEST_F(SculptBrushTest, testGrowFillsGap) {
	// A 3x3 surface with a missing center should be filled at high strength
	voxel::RawVolume volume(voxel::Region(-5, 5));

	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			if (x == 0 && z == 0) {
				continue; // Leave gap at center
			}
			volume.setVoxel(x, 0, z, selectedVoxel());
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SculptBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSculptMode(SculptMode::Grow);
	// strength=1.0 -> addThreshold=1 (any air touching a solid voxel)
	brush.setStrength(1.0f);
	brush.setIterations(1);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeSculpt(brush, node, ctx);
	brush.endBrush(ctx);

	// The gap at (0,0,0) has 4 solid face-neighbors -> 4 >= 1 -> filled
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 0, 0).getMaterial()))
		<< "Gap at (0,0,0) should be filled by grow";

	brush.shutdown();
}

TEST_F(SculptBrushTest, testFlattenRemovesLayers) {
	// A 3x3x3 cube selected, flatten from PositiveY should remove top layer per iteration
	voxel::RawVolume volume(voxel::Region(-5, 5));

	for (int x = -1; x <= 1; ++x) {
		for (int y = 0; y <= 2; ++y) {
			for (int z = -1; z <= 1; ++z) {
				volume.setVoxel(x, y, z, selectedVoxel());
			}
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SculptBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSculptMode(SculptMode::Flatten);
	brush.setIterations(1);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveY;

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeSculpt(brush, node, ctx);
	brush.endBrush(ctx);

	// Top layer (y=2) should be removed, y=0 and y=1 should remain
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			EXPECT_TRUE(voxel::isAir(volume.voxel(x, 2, z).getMaterial()))
				<< "Top layer voxel at (" << x << ",2," << z << ") should be removed";
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 1, z).getMaterial()))
				<< "Middle layer voxel at (" << x << ",1," << z << ") should remain";
			EXPECT_TRUE(voxel::isBlocked(volume.voxel(x, 0, z).getMaterial()))
				<< "Bottom layer voxel at (" << x << ",0," << z << ") should remain";
		}
	}

	brush.shutdown();
}

TEST_F(SculptBrushTest, testFlattenReversible) {
	// Reducing iterations should restore layers from snapshot
	voxel::RawVolume volume(voxel::Region(-5, 5));

	for (int x = -1; x <= 1; ++x) {
		for (int y = 0; y <= 2; ++y) {
			for (int z = -1; z <= 1; ++z) {
				volume.setVoxel(x, y, z, selectedVoxel());
			}
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SculptBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSculptMode(SculptMode::Flatten);
	brush.setIterations(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorFace = voxel::FaceNames::PositiveY;

	ASSERT_TRUE(brush.beginBrush(ctx));
	executeSculpt(brush, node, ctx);

	// 2 iterations should remove y=2 and y=1
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 2, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 1, 0).getMaterial()));

	// Reduce to 1 iteration - should restore y=1 from snapshot
	brush.setIterations(1);
	executeSculpt(brush, node, ctx);

	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 2, 0).getMaterial()))
		<< "Top layer should still be removed";
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(0, 1, 0).getMaterial()))
		<< "Middle layer should be restored when iterations reduced";

	brush.endBrush(ctx);
	brush.shutdown();
}

} // namespace voxedit
