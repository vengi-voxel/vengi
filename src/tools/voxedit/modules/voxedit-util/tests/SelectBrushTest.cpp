/**
 * @file
 */

#include "../modifier/brush/SelectBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxedit {

class SelectBrushTest : public app::AbstractTest {
protected:
	void prepare(SelectBrush &brush, BrushContext &ctx, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		ctx.cursorPosition = mins;
		ctx.cursorFace = voxel::FaceNames::PositiveX;
		EXPECT_TRUE(brush.beginBrush(ctx));
		EXPECT_TRUE(brush.active());
		ctx.cursorPosition = maxs;
		brush.step(ctx);
	}

	void executeSelect(SelectBrush &brush, scenegraph::SceneGraphNode &node, BrushContext &ctx,
					   ModifierType modifierType = ModifierType::Override) {
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, modifierType);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
		brush.endBrush(ctx);
	}
};

TEST_F(SelectBrushTest, testSelectModeAll) {
	voxel::RawVolume volume({-5, 5});
	// Fill volume with some voxels
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::All);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.referencePos = glm::ivec3(-2, -2, -2);

	// Select the entire volume region which includes all solid voxels
	prepare(brush, ctx, glm::ivec3(-2, -2, -2), glm::ivec3(2, 2, 2));
	executeSelect(brush, node, ctx, ModifierType::Override);

	// SelectMode::All uses VisitSolid, so ALL solid voxels should be selected (including interior)
	// Interior voxel should be selected
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Interior voxel at (0,0,0) should be selected";

	// Surface voxels at the boundary of the volume should also be selected
	EXPECT_TRUE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (2,0,0) should be selected";
	EXPECT_TRUE((volume.voxel(-2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (-2,0,0) should be selected";
	EXPECT_TRUE((volume.voxel(0, 2, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (0,2,0) should be selected";

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeSameColor) {
	voxel::RawVolume volume({-5, 5});
	// Fill volume with voxels of different colors
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				// Use color 1 for even x, color 2 for odd x
				uint8_t color = (x % 2 == 0) ? 1 : 2;
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, color));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::SameColor);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.referencePos = glm::ivec3(-2, -2, -2);
	// Set the hit cursor voxel to color 1 (even x voxels)
	ctx.hitCursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	prepare(brush, ctx, glm::ivec3(-2, -2, -2), glm::ivec3(2, 2, 2));
	executeSelect(brush, node, ctx, ModifierType::Override);

	// Only voxels with color 1 (even x) should be selected
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				bool shouldBeSelected = (x % 2 == 0);
				bool isSelected = (volume.voxel(x, y, z).getFlags() & voxel::FlagOutline) != 0;
				EXPECT_EQ(shouldBeSelected, isSelected)
					<< "Voxel at " << x << "," << y << "," << z << " (color "
					<< (int)volume.voxel(x, y, z).getColor() << ") selection mismatch";
			}
		}
	}
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeConnected) {
	voxel::RawVolume volume({-5, 5});
	// Create two separate connected regions with the same color
	// Region 1: center pillar
	for (int y = -2; y <= 2; ++y) {
		volume.setVoxel(0, y, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	}
	// Region 2: disconnected voxel
	volume.setVoxel(3, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Connected);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	// Start from center of the pillar
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.referencePos = glm::ivec3(0, 0, 0);
	// Set the hit cursor voxel to the pillar color
	ctx.hitCursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	// Reset cursor position to the starting voxel for connected flood fill
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	executeSelect(brush, node, ctx, ModifierType::Override);

	// The pillar voxels should be selected (connected)
	for (int y = -2; y <= 2; ++y) {
		EXPECT_TRUE((volume.voxel(0, y, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Pillar voxel at y=" << y << " should be selected";
	}

	// The disconnected voxel should NOT be selected
	EXPECT_FALSE((volume.voxel(3, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Disconnected voxel should not be selected";
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeVisible) {
	voxel::RawVolume volume({-5, 5});
	// Create a solid cube - only surface voxels are visible
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Surface);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.referencePos = glm::ivec3(-2, -2, -2);

	prepare(brush, ctx, glm::ivec3(-2, -2, -2), glm::ivec3(2, 2, 2));
	executeSelect(brush, node, ctx, ModifierType::Override);

	// Interior voxels should NOT be selected (they are invisible)
	for (int z = -1; z <= 1; ++z) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				EXPECT_FALSE((volume.voxel(x, y, z).getFlags() & voxel::FlagOutline) != 0)
					<< "Interior voxel at " << x << "," << y << "," << z << " should not be selected";
			}
		}
	}

	// Surface voxels should be selected
	// Check a few surface voxels on different faces
	EXPECT_TRUE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (2,0,0) should be selected";
	EXPECT_TRUE((volume.voxel(-2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (-2,0,0) should be selected";
	EXPECT_TRUE((volume.voxel(0, 2, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (0,2,0) should be selected";
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectRemove) {
	voxel::RawVolume volume({-5, 5});
	// Fill volume with voxels, all selected
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, 0);
				v.setFlags(voxel::FlagOutline);
				volume.setVoxel(x, y, z, v);
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::All);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.referencePos = glm::ivec3(-2, -2, -2);

	// Deselect the entire volume region
	prepare(brush, ctx, glm::ivec3(-2, -2, -2), glm::ivec3(2, 2, 2));
	executeSelect(brush, node, ctx, ModifierType::Erase);

	// SelectMode::All uses VisitSolid, so ALL solid voxels should be deselected (including interior)
	// Surface voxels at the boundary should be deselected
	EXPECT_FALSE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (2,0,0) should be deselected";
	EXPECT_FALSE((volume.voxel(-2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (-2,0,0) should be deselected";

	// Interior voxel should also be deselected (VisitSolid visits all solid voxels)
	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Interior voxel at (0,0,0) should be deselected";

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeFlatSurface) {
	// 5x5 flat floor at y=0 with PositiveY face exposed (nothing above)
	voxel::RawVolume volume({-5, 5});
	for (int z = -2; z <= 2; ++z) {
		for (int x = -2; x <= 2; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::FlatSurface);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	// All 5x5 floor voxels should be selected
	for (int z = -2; z <= 2; ++z) {
		for (int x = -2; x <= 2; ++x) {
			EXPECT_TRUE((volume.voxel(x, 0, z).getFlags() & voxel::FlagOutline) != 0)
				<< "Floor voxel at (" << x << ",0," << z << ") should be selected";
		}
	}
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeFlatSurface_stopsAtCoveredVoxel) {
	// A line of floor voxels at y=0 along the X axis
	// One voxel's PositiveY face is blocked by a voxel above it
	voxel::RawVolume volume({-5, 5});
	for (int x = -3; x <= 3; ++x) {
		volume.setVoxel(x, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	}
	// Block PositiveY face of (0,0,0) by placing a solid voxel above it
	volume.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::FlatSurface);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	// Start from the left end of the line
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(-3, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	// Left side (before the blocked voxel) should be selected
	for (int x = -3; x <= -1; ++x) {
		EXPECT_TRUE((volume.voxel(x, 0, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Voxel at (" << x << ",0,0) should be selected";
	}
	// The voxel with a blocked PositiveY face should NOT be selected
	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Covered voxel (0,0,0) should not be selected";
	// Right side (beyond the barrier) should also NOT be selected
	for (int x = 1; x <= 3; ++x) {
		EXPECT_FALSE((volume.voxel(x, 0, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Voxel at (" << x << ",0,0) should not be selected (beyond blocked voxel)";
	}
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeFlatSurface_deviation) {
	// Stepped floor: y=0 for x in [-2..0], y=1 for x in [1..3], both along z=0
	voxel::RawVolume volume({-5, 5});
	for (int x = -2; x <= 0; ++x) {
		volume.setVoxel(x, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	}
	for (int x = 1; x <= 3; ++x) {
		volume.setVoxel(x, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::FlatSurface);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// With deviation=0: only the lower floor (y=0) should be selected
	brush.setFlatDeviation(0);
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	for (int x = -2; x <= 0; ++x) {
		EXPECT_TRUE((volume.voxel(x, 0, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Lower voxel at (" << x << ",0,0) should be selected with deviation=0";
	}
	for (int x = 1; x <= 3; ++x) {
		EXPECT_FALSE((volume.voxel(x, 1, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Upper voxel at (" << x << ",1,0) should NOT be selected with deviation=0";
	}

	// Clear selection flags for the next part of the test
	for (int x = -2; x <= 0; ++x) {
		voxel::Voxel v = volume.voxel(x, 0, 0);
		v.setFlags(v.getFlags() & ~voxel::FlagOutline);
		volume.setVoxel(x, 0, 0, v);
	}

	// With deviation=1: both floors should be reachable (step of 1 in Y from start)
	brush.setFlatDeviation(1);
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	for (int x = -2; x <= 0; ++x) {
		EXPECT_TRUE((volume.voxel(x, 0, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Lower voxel at (" << x << ",0,0) should be selected with deviation=1";
	}
	for (int x = 1; x <= 3; ++x) {
		EXPECT_TRUE((volume.voxel(x, 1, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Upper voxel at (" << x << ",1,0) should be selected with deviation=1";
	}

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeFlatSurface_invalidFace) {
	voxel::RawVolume volume({-5, 5});
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::FlatSurface);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::Max; // invalid face
	executeSelect(brush, node, ctx);

	// Nothing should be selected when face is invalid
	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "No voxel should be selected when face is Max";
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeCircle) {
	// Create a flat 11x11 floor at y=0
	voxel::RawVolume volume({-5, 5});
	for (int z = -5; z <= 5; ++z) {
		for (int x = -5; x <= 5; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Circle);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();

	// Click at center (0,0,0), drag to (3,0,0) -> radius = 3
	// Set cursorFace before prepare() so _aabbFace locks to PositiveY
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_TRUE(brush.active());
	ctx.cursorPosition = glm::ivec3(3, 0, 0);
	brush.step(ctx);
	executeSelect(brush, node, ctx, ModifierType::Override);

	// Center should be selected
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Center voxel should be selected";

	// Voxels within radius 3 should be selected
	EXPECT_TRUE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel within radius should be selected";
	EXPECT_TRUE((volume.voxel(0, 0, 2).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel within radius should be selected";
	EXPECT_TRUE((volume.voxel(3, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel at exactly the radius should be selected";

	// Voxels outside the radius should NOT be selected
	EXPECT_FALSE((volume.voxel(5, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel far outside radius should not be selected";
	EXPECT_FALSE((volume.voxel(0, 0, 5).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel far outside radius should not be selected";

	// Diagonal at distance sqrt(3^2+3^2) = ~4.24 > 3 should NOT be selected
	EXPECT_FALSE((volume.voxel(3, 0, 3).getFlags() & voxel::FlagOutline) != 0)
		<< "Diagonal voxel outside radius should not be selected";

	// Diagonal at distance sqrt(2^2+2^2) = ~2.83 <= 3 should be selected
	EXPECT_TRUE((volume.voxel(2, 0, 2).getFlags() & voxel::FlagOutline) != 0)
		<< "Diagonal voxel within radius should be selected";

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeCircle_ellipseParams) {
	voxel::RawVolume volume({-5, 5});
	for (int z = -5; z <= 5; ++z) {
		for (int x = -5; x <= 5; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Circle);
	EXPECT_FALSE(brush.ellipseValid());

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	// Set cursorFace before beginBrush so _aabbFace locks to PositiveY
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_TRUE(brush.active());
	ctx.cursorPosition = glm::ivec3(3, 0, 0);
	brush.step(ctx);
	executeSelect(brush, node, ctx, ModifierType::Override);

	// After execution, ellipse params should be cached
	EXPECT_TRUE(brush.ellipseValid());
	EXPECT_EQ(brush.ellipseCenter(), glm::ivec3(0, 0, 0));
	EXPECT_EQ(brush.ellipseRadiusU(), 3);
	EXPECT_EQ(brush.ellipseRadiusV(), 3);
	EXPECT_EQ(brush.ellipseFace(), voxel::FaceNames::PositiveY);

	// Changing select mode should invalidate ellipse
	brush.setSelectMode(SelectMode::All);
	EXPECT_FALSE(brush.ellipseValid());

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeCircle_invalidFace) {
	voxel::RawVolume volume({-5, 5});
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Circle);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	// Set invalid face before beginBrush so _aabbFace locks to Max
	ctx.cursorFace = voxel::FaceNames::Max;
	ctx.cursorPosition = glm::ivec3(-5, -5, -5);
	EXPECT_TRUE(brush.beginBrush(ctx));
	ctx.cursorPosition = glm::ivec3(5, 5, 5);
	brush.step(ctx);
	executeSelect(brush, node, ctx);

	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "No voxel should be selected when face is invalid";
	EXPECT_FALSE(brush.ellipseValid());
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeCircle_onlySurface) {
	// Create a solid 5x5x5 cube - circle should only select surface voxels
	voxel::RawVolume volume({-5, 5});
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Circle);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	// Set cursorFace before beginBrush so _aabbFace locks to PositiveY
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_TRUE(brush.active());
	ctx.cursorPosition = glm::ivec3(5, 0, 0);
	brush.step(ctx);
	executeSelect(brush, node, ctx, ModifierType::Override);

	// Interior voxels should NOT be selected (only surface voxels)
	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Interior voxel should not be selected";

	// Surface voxels within radius should be selected
	EXPECT_TRUE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel within radius should be selected";

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeSlope_flatSurface) {
	// 5x5 flat floor at y=0 -gradient is zero everywhere, so any angle should select all
	voxel::RawVolume volume({-5, 5});
	for (int z = -2; z <= 2; ++z) {
		for (int x = -2; x <= 2; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Slope);
	brush.setSlopeDeviation(10);
	brush.setSlopeSampleDistance(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	// All floor voxels should be selected (gradient is zero everywhere)
	for (int z = -2; z <= 2; ++z) {
		for (int x = -2; x <= 2; ++x) {
			EXPECT_TRUE((volume.voxel(x, 0, z).getFlags() & voxel::FlagOutline) != 0)
				<< "Floor voxel at (" << x << ",0," << z << ") should be selected";
		}
	}
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeSlope_stopsAtWall) {
	// Floor at y=0 and a vertical wall at x=3. The wall voxels aren't surface
	// voxels on the +Y face, so slope fill from the floor should not include them.
	voxel::RawVolume volume({-5, 5});
	// Floor
	for (int z = -1; z <= 1; ++z) {
		for (int x = -3; x <= 3; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	// Wall going up from x=3 (blocks +Y face of floor voxels at x=3 for y>=1)
	for (int y = 1; y <= 4; ++y) {
		for (int z = -1; z <= 1; ++z) {
			volume.setVoxel(3, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Slope);
	brush.setSlopeDeviation(45);
	brush.setSlopeSampleDistance(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	// Floor voxels should be selected
	EXPECT_TRUE((volume.voxel(-3, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Floor voxel at (-3,0,0) should be selected";
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Floor voxel at (0,0,0) should be selected";

	// Wall voxels should NOT be selected (they face +X, not +Y -not surface voxels for this face)
	EXPECT_FALSE((volume.voxel(3, 2, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Wall voxel at (3,2,0) should not be selected";
	EXPECT_FALSE((volume.voxel(3, 4, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Wall voxel at (3,4,0) should not be selected";

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeSlope_staircase) {
	// 45-degree staircase: each step goes +1 in X and +1 in Y.
	// All steps have the same gradient, so slope fill should select them all.
	voxel::RawVolume volume({-5, 10});
	// Build a filled staircase so each step has +Y exposed
	for (int step = 0; step < 6; ++step) {
		// Each step column goes from y=0 up to y=step
		for (int y = 0; y <= step; ++y) {
			for (int z = -1; z <= 1; ++z) {
				volume.setVoxel(step, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Slope);
	brush.setSlopeDeviation(10); // Small threshold -staircase gradient should be consistent
	brush.setSlopeSampleDistance(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(10, 10, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	// Each stair top should be selected (consistent 45-degree slope)
	for (int step = 0; step < 6; ++step) {
		EXPECT_TRUE((volume.voxel(step, step, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Stair top at (" << step << "," << step << ",0) should be selected";
	}

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModeSlope_disconnectedNotSelected) {
	// Two separate flat surfaces -slope fill should not jump the gap
	voxel::RawVolume volume({-5, 5});
	// Surface 1 at x=[-2..0]
	for (int x = -2; x <= 0; ++x) {
		volume.setVoxel(x, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	}
	// Surface 2 at x=[2..4] -gap at x=1
	for (int x = 2; x <= 4; ++x) {
		volume.setVoxel(x, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SelectBrush brush;
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Slope);
	brush.setSlopeDeviation(90);
	brush.setSlopeSampleDistance(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	prepare(brush, ctx, glm::ivec3(-5, -5, -5), glm::ivec3(5, 5, 5));
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	executeSelect(brush, node, ctx);

	// Surface 1 should be selected
	for (int x = -2; x <= 0; ++x) {
		EXPECT_TRUE((volume.voxel(x, 0, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Voxel at (" << x << ",0,0) should be selected";
	}

	// Surface 2 should NOT be selected (disconnected)
	for (int x = 2; x <= 4; ++x) {
		EXPECT_FALSE((volume.voxel(x, 0, 0).getFlags() & voxel::FlagOutline) != 0)
			<< "Disconnected voxel at (" << x << ",0,0) should not be selected";
	}

	brush.shutdown();
}

} // namespace voxedit
