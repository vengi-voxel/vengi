/**
 * @file
 */

#include "../modifier/brush/SelectBrush.h"
#include "../modifier/brush/LUASelectionMode.h"
#include "app/tests/AbstractTest.h"
#include "io/Filesystem.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"
#include "voxelutil/VolumeSelect.h"

namespace voxedit {

class SelectBrushTest : public app::AbstractTest {
protected:
	bool onInitApp() override {
		app::AbstractTest::onInitApp();
		return _testApp->filesystem()->registerPath("selectionmodes/");
	}

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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
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
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("slope"));
	luaMode.parameters().push_back("10");
	luaMode.parameters().push_back("2");

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

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
	luaMode.shutdown();
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
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("slope"));
	luaMode.parameters().push_back("45");
	luaMode.parameters().push_back("2");

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

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
	luaMode.shutdown();
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
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("slope"));
	luaMode.parameters().push_back("10");
	luaMode.parameters().push_back("2");

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

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
	luaMode.shutdown();
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
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("slope"));
	luaMode.parameters().push_back("90");
	luaMode.parameters().push_back("2");

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

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
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testHoleRim2D_wellOpening) {
	// Flat XZ floor at y=0 with a 3x3 hole in the middle (x=3..5, z=3..5 is air)
	// Rim voxels border the hole in the XZ plane at y=0
	voxel::RawVolume volume({-1, 10});
	for (int z = 0; z <= 8; ++z) {
		for (int x = 0; x <= 8; ++x) {
			const bool isHole = (x >= 3 && x <= 5 && z >= 3 && z <= 5);
			if (!isHole) {
				volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("holerim2d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click the top face (+Y) of a rim voxel at (2,0,4) - left edge of hole
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(2, 0, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// Rim voxels: all solid voxels adjacent to the hole in the XZ plane at y=0
	// (x=2,z=3..5), (x=6,z=3..5), (z=2,x=3..5), (z=6,x=3..5) + corners
	EXPECT_NE(volume.voxel(2, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Left rim voxel should be selected";
	EXPECT_NE(volume.voxel(6, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Right rim voxel should be selected";
	EXPECT_NE(volume.voxel(4, 0, 2).getFlags() & voxel::FlagOutline, 0)
		<< "Front rim voxel should be selected";
	EXPECT_NE(volume.voxel(4, 0, 6).getFlags() & voxel::FlagOutline, 0)
		<< "Back rim voxel should be selected";

	// Interior floor voxels far from hole should NOT be selected
	EXPECT_EQ(volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline, 0)
		<< "Floor voxel far from hole should not be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testHoleRim2D_tubeWallRadialClick) {
	// Hollow square tube running along Z (z=0..8).
	// Cross-section at each Z: solid ring at x=3,x=5 (y=3..5) and y=3,y=5 (x=3..5),
	// hollow interior at (4,4,z). The XY plane at any fixed Z is fully bounded.
	// User clicks the inner face (+X) of the left wall at x=3 - a radial face
	// perpendicular to the hole plane. The algorithm must find the XY cross-section
	// via the face-direction seed (all-axes trial) and select the ring.
	voxel::RawVolume volume({0, 9});
	for (int z = 0; z <= 8; ++z) {
		// Left and right walls
		for (int y = 3; y <= 5; ++y) {
			volume.setVoxel(3, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			volume.setVoxel(5, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
		// Top and bottom walls
		for (int x = 3; x <= 5; ++x) {
			volume.setVoxel(x, 3, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			volume.setVoxel(x, 5, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("holerim2d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click left wall at (3,4,4) using +X face (pointing into the hollow)
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorPosition = glm::ivec3(3, 4, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// The face-direction seed lands at (4,4,4) = hollow interior.
	// Only the XY plane at Z=4 is bounded (other planes are open at tube ends).
	// The rim in XY at Z=4 is the 8-voxel ring around the 1-voxel interior.
	EXPECT_NE(volume.voxel(5, 4, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Opposite wall (x=5) should be selected as part of the cross-section rim";
	EXPECT_NE(volume.voxel(4, 3, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Bottom wall (y=3) should be selected as part of the cross-section rim";
	EXPECT_NE(volume.voxel(4, 5, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Top wall (y=5) should be selected as part of the cross-section rim";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testHoleRim2D_openEdgeNotSelected) {
	// Flat XZ floor at y=0 with a notch cut into the edge (x=3..5, z=0..2 is air)
	// The notch connects to the volume boundary - not a closed hole, should not select
	voxel::RawVolume volume({-1, 10});
	for (int z = 0; z <= 8; ++z) {
		for (int x = 0; x <= 8; ++x) {
			const bool isNotch = (x >= 3 && x <= 5 && z <= 2);
			if (!isNotch) {
				volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("holerim2d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	// Click a voxel bordering the open notch
	ctx.cursorPosition = glm::ivec3(2, 0, 2);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// Open notch connects to volume boundary - no rim should be selected
	EXPECT_EQ(volume.voxel(2, 0, 2).getFlags() & voxel::FlagOutline, 0)
		<< "Voxel adjacent to open notch should not be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testLassoContains_insideSquare) {
	// 4-vertex square polygon on the XZ plane (uAxis=X, vAxis=Z)
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(0, 0, 0));
	path.push_back(glm::ivec3(4, 0, 0));
	path.push_back(glm::ivec3(4, 0, 4));
	path.push_back(glm::ivec3(0, 0, 4));
	const int uAxis = 0; // X
	const int vAxis = 2; // Z

	EXPECT_TRUE(voxelutil::lassoContains(path, 2, 2, uAxis, vAxis));
	EXPECT_FALSE(voxelutil::lassoContains(path, 6, 6, uAxis, vAxis));
	EXPECT_FALSE(voxelutil::lassoContains(path, -1, 2, uAxis, vAxis));
	EXPECT_FALSE(voxelutil::lassoContains(path, 2, -1, uAxis, vAxis));
}

TEST_F(SelectBrushTest, testLassoContains_insideTriangle) {
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(0, 0, 0));
	path.push_back(glm::ivec3(6, 0, 0));
	path.push_back(glm::ivec3(3, 0, 6));
	const int uAxis = 0; // X
	const int vAxis = 2; // Z

	EXPECT_TRUE(voxelutil::lassoContains(path, 3, 2, uAxis, vAxis));
	EXPECT_FALSE(voxelutil::lassoContains(path, 0, 6, uAxis, vAxis));
	EXPECT_FALSE(voxelutil::lassoContains(path, 6, 6, uAxis, vAxis));
}

TEST_F(SelectBrushTest, testLassoSelectSquareSurface) {
	// Flat XZ surface at y=0, spanning x=[0..8], z=[0..8]
	voxel::RawVolume volume({-1, 10});
	for (int z = 0; z <= 8; ++z) {
		for (int x = 0; x <= 8; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Lasso);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	ctx.cursorFace = voxel::FaceNames::PositiveY;

	// Click vertices of a square: (2,0,2), (6,0,2), (6,0,6), (2,0,6)
	// then close by clicking (2,0,2) again.
	// Note: a triangle with vertices (2,0,2),(6,0,2),(6,0,6) would have (3,0,3) and
	// (4,0,4) on the hypotenuse (z=x diagonal) - a boundary case for lassoContains.
	// Using 4 vertices gives a square where both interior test points are strictly inside.
	const glm::ivec3 vertices[] = {
		glm::ivec3(2, 0, 2), glm::ivec3(6, 0, 2), glm::ivec3(6, 0, 6), glm::ivec3(2, 0, 6)
	};
	for (const glm::ivec3 &vertex : vertices) {
		ctx.cursorPosition = vertex;
		EXPECT_TRUE(brush.beginBrush(ctx));
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, ModifierType::Override);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
		brush.endBrush(ctx);
	}

	// Close by clicking near first vertex
	ctx.cursorPosition = glm::ivec3(2, 0, 2);
	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_FALSE(brush.lassoAccumulating()) << "Polygon should be closed";

	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);
	brush.preExecute(ctx, wrapper.volume());
	brush.execute(sceneGraph, wrapper, ctx);
	brush.endBrush(ctx);

	// Interior voxels should be selected
	EXPECT_NE(volume.voxel(4, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Interior voxel (4,0,4) should be selected";
	EXPECT_NE(volume.voxel(3, 0, 3).getFlags() & voxel::FlagOutline, 0)
		<< "Interior voxel (3,0,3) should be selected";

	// Voxels outside the polygon should not be selected
	EXPECT_EQ(volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline, 0)
		<< "Voxel (0,0,0) outside polygon should not be selected";
	EXPECT_EQ(volume.voxel(8, 0, 8).getFlags() & voxel::FlagOutline, 0)
		<< "Voxel (8,0,8) outside polygon should not be selected";

	brush.shutdown();
}

TEST_F(SelectBrushTest, testLassoCancel_edgeMarksReverted) {
	// Draw two vertices (producing an edge), cancel, verify no FlagOutline remains
	voxel::RawVolume volume({-1, 10});
	for (int z = 0; z <= 8; ++z) {
		for (int x = 0; x <= 8; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Lasso);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	ctx.cursorFace = voxel::FaceNames::PositiveY;

	// Click 1: first vertex (no edge yet)
	ctx.cursorPosition = glm::ivec3(2, 0, 2);
	EXPECT_TRUE(brush.beginBrush(ctx));
	{
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, ModifierType::Override);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
	}
	brush.endBrush(ctx);

	// Click 2: second vertex (edge 1->2 drawn on real volume)
	ctx.cursorPosition = glm::ivec3(6, 0, 2);
	EXPECT_TRUE(brush.beginBrush(ctx));
	{
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, ModifierType::Override);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
	}
	brush.endBrush(ctx);

	EXPECT_TRUE(brush.hasPendingChanges()) << "Should have pending edge marks after two clicks";

	// Cancel: revert edge marks (simulates ESC / cancellasso command)
	brush.revertChanges(&volume);
	brush.invalidateLasso();
	EXPECT_FALSE(brush.lassoAccumulating());

	// No voxels should remain selected after cancel
	for (int z = 0; z <= 8; ++z) {
		for (int x = 0; x <= 8; ++x) {
			EXPECT_EQ(volume.voxel(x, 0, z).getFlags() & voxel::FlagOutline, 0)
				<< "Voxel (" << x << ",0," << z << ") should not be selected after cancel";
		}
	}

	brush.shutdown();
}

TEST_F(SelectBrushTest, testLassoUndoVertex_revertsLastEdge) {
	// Flat XZ surface at y=0, spanning x=[0..8], z=[0..8]
	voxel::RawVolume volume({-1, 10});
	for (int z = 0; z <= 8; ++z) {
		for (int x = 0; x <= 8; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Lasso);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	ctx.cursorFace = voxel::FaceNames::PositiveY;

	auto clickVertex = [&](const glm::ivec3 &pos) {
		ctx.cursorPosition = pos;
		EXPECT_TRUE(brush.beginBrush(ctx));
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, ModifierType::Override);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
		brush.endBrush(ctx);
	};

	// Click 3 vertices: (2,0,2) -> (6,0,2) -> (6,0,6)
	clickVertex(glm::ivec3(2, 0, 2));
	clickVertex(glm::ivec3(6, 0, 2));
	clickVertex(glm::ivec3(6, 0, 6));

	EXPECT_EQ((int)brush.lassoPath().size(), 3);
	EXPECT_TRUE(brush.hasPendingChanges());

	// Edge (6,0,2)-(6,0,6) should be marked: x=6 along z=[2..6]
	EXPECT_NE(volume.voxel(6, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Voxel on second edge should be selected";

	// Undo the last vertex - path shrinks to 2, edge to (6,0,6) should vanish
	voxel::Region dummy = voxel::Region::InvalidRegion;
	brush.revertChanges(&volume);
	brush.popLastLassoPathEntry();
	EXPECT_EQ((int)brush.lassoPath().size(), 2);
	brush.redrawEdgesOnVolume(&volume, volume.region(), dummy);

	// Only the first edge (2,0,2)-(6,0,2) should be marked now
	EXPECT_NE(volume.voxel(4, 0, 2).getFlags() & voxel::FlagOutline, 0)
		<< "Voxel on first edge should still be selected";
	// The second edge voxels (x=6 at z=4,5,6) should no longer be marked
	EXPECT_EQ(volume.voxel(6, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Voxel on removed second edge should not be selected after vertex undo";

	brush.shutdown();
}

TEST_F(SelectBrushTest, testColumnRim2D_pillar) {
	// 3x3 solid pillar at x=3..5, z=3..5 standing on its own (surrounded by air in the XZ plane at y=4)
	// Clicking the top face (+Y) of any pillar voxel should select all 9 cross-section voxels
	voxel::RawVolume volume({0, 9});
	for (int z = 3; z <= 5; ++z) {
		for (int x = 3; x <= 5; ++x) {
			volume.setVoxel(x, 4, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("columnrim2d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click the top face (+Y) of the center pillar voxel
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(4, 4, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// All 9 pillar voxels in the XZ plane at y=4 should be selected
	int selectedCount = 0;
	for (int z = 3; z <= 5; ++z) {
		for (int x = 3; x <= 5; ++x) {
			if (volume.voxel(x, 4, z).getFlags() & voxel::FlagOutline) {
				++selectedCount;
			}
		}
	}
	EXPECT_EQ(selectedCount, 9) << "All 9 pillar cross-section voxels should be selected";

	// Air voxels should not be selected
	EXPECT_EQ(volume.voxel(0, 4, 0).getFlags() & voxel::FlagOutline, 0)
		<< "Air voxel outside pillar should not be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testColumnRim2D_largeFloor) {
	// Large solid floor fills the XZ plane at y=0 all the way to the volume boundary.
	// Clicking any floor voxel should select nothing because the solid region is unbounded
	// (BFS tries to step outside the volume region and sets bounded=false).
	// Volume {0,9} so region is (0,0,0)-(9,9,9). Floor at x=0..9, z=0..9 fills the boundary.
	voxel::RawVolume volume({0, 9});
	for (int z = 0; z <= 9; ++z) {
		for (int x = 0; x <= 9; ++x) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("columnrim2d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click the top face (+Y) of a center floor voxel
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(4, 0, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// No floor voxel should be selected (region is unbounded)
	EXPECT_EQ(volume.voxel(4, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Floor voxel should not be selected (unbounded solid region)";
	EXPECT_EQ(volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline, 0)
		<< "Corner floor voxel should not be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testColumnRim2D_sideClick) {
	// 3x3 column at x=3..5, z=3..5, y=0..9 (touches volume boundary at y=0 and y=9).
	// Clicking the +X side face: the YZ wall at x=5 is unbounded (reaches y=0 and y=9),
	// so the algorithm falls back to the XZ cross-section at y=4 which is bounded.
	voxel::RawVolume volume({0, 10});
	for (int y = 0; y <= 9; ++y) {
		for (int z = 3; z <= 5; ++z) {
			for (int x = 3; x <= 5; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("columnrim2d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click the +X side face of the column at mid-height
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorPosition = glm::ivec3(5, 4, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// XZ cross-section at y=4 should be selected (fallback from unbounded YZ wall)
	int selectedCount = 0;
	for (int z = 3; z <= 5; ++z) {
		for (int x = 3; x <= 5; ++x) {
			if (volume.voxel(x, 4, z).getFlags() & voxel::FlagOutline) {
				++selectedCount;
			}
		}
	}
	EXPECT_EQ(selectedCount, 9) << "All 9 XZ cross-section voxels should be selected via fallback";

	// Voxels at other Y levels should not be selected
	EXPECT_EQ(volume.voxel(4, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Column voxel at floor level should not be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testColumnRim2D_singleVoxel) {
	// A single isolated solid voxel - a bounded region of size 1
	voxel::RawVolume volume({0, 9});
	volume.setVoxel(4, 4, 4, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("columnrim2d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(4, 4, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	EXPECT_NE(volume.voxel(4, 4, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Single isolated voxel should be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testHoleRim3D_wellOpening) {
	// Flat XZ floor at y=0 with a 1x1 hole at x=4,z=4.
	// HoleRim3D should find the same 4 cardinal rim voxels as HoleRim2D.
	voxel::RawVolume volume({-1, 10});
	for (int z = 0; z <= 8; ++z) {
		for (int x = 0; x <= 8; ++x) {
			const bool isHole = (x == 4 && z == 4);
			if (!isHole) {
				volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("holerim3d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click +Y face of a cardinal rim voxel adjacent to the hole
	ctx.cursorFace = voxel::FaceNames::PositiveY;
	ctx.cursorPosition = glm::ivec3(3, 0, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// The 4 cardinal rim voxels around the 1x1 hole should be selected
	EXPECT_NE(volume.voxel(3, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Left rim voxel should be selected";
	EXPECT_NE(volume.voxel(5, 0, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Right rim voxel should be selected";
	EXPECT_NE(volume.voxel(4, 0, 3).getFlags() & voxel::FlagOutline, 0)
		<< "Front rim voxel should be selected";
	EXPECT_NE(volume.voxel(4, 0, 5).getFlags() & voxel::FlagOutline, 0)
		<< "Back rim voxel should be selected";

	// Voxels far from the hole should not be selected
	EXPECT_EQ(volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline, 0)
		<< "Floor voxel far from hole should not be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testHoleRim3D_hollowTube) {
	// Hollow square tube along Z (z=0..8), 3x3 outer cross-section with 1x1 hollow at (4,4,z).
	// Click the inner face (+X) of the left wall at x=3. Air seed is at (4,4,4).
	// The minimal rim should be the 8-voxel ring at Z=4.
	voxel::RawVolume volume({0, 9});
	for (int z = 0; z <= 8; ++z) {
		for (int y = 3; y <= 5; ++y) {
			volume.setVoxel(3, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			volume.setVoxel(5, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
		for (int x = 3; x <= 5; ++x) {
			volume.setVoxel(x, 3, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			volume.setVoxel(x, 5, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("holerim3d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click inner +X face of left wall at (3,4,4)
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorPosition = glm::ivec3(3, 4, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// The minimal rim encircling the 1x1 hollow at Z=4 should be selected:
	// 8 voxels of the XY ring at Z=4
	EXPECT_NE(volume.voxel(3, 4, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Left wall voxel should be selected";
	EXPECT_NE(volume.voxel(5, 4, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Right wall voxel should be selected";
	EXPECT_NE(volume.voxel(4, 3, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Bottom wall voxel should be selected";
	EXPECT_NE(volume.voxel(4, 5, 4).getFlags() & voxel::FlagOutline, 0)
		<< "Top wall voxel should be selected";

	// Voxels at different Z slices should not be selected
	EXPECT_EQ(volume.voxel(3, 4, 0).getFlags() & voxel::FlagOutline, 0)
		<< "Tube end voxel at Z=0 should not be selected";
	EXPECT_EQ(volume.voxel(3, 4, 8).getFlags() & voxel::FlagOutline, 0)
		<< "Tube end voxel at Z=8 should not be selected";

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testHoleRim3D_solidVoxelNoSelect) {
	// Clicking a voxel with no adjacent air in the face direction should select nothing.
	// Build a solid 3x3x3 block - the center voxel has no air face neighbor.
	voxel::RawVolume volume({0, 9});
	for (int z = 3; z <= 5; ++z) {
		for (int y = 3; y <= 5; ++y) {
			for (int x = 3; x <= 5; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("holerim3d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click +X face of center voxel (4,4,4): air seed at (5,4,4) is inside the solid block
	ctx.cursorFace = voxel::FaceNames::PositiveX;
	ctx.cursorPosition = glm::ivec3(4, 4, 4);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// Air seed (5,4,4) is solid - algorithm should return early, nothing selected
	for (int z = 3; z <= 5; ++z) {
		for (int y = 3; y <= 5; ++y) {
			for (int x = 3; x <= 5; ++x) {
				EXPECT_EQ(volume.voxel(x, y, z).getFlags() & voxel::FlagOutline, 0)
					<< "No voxel should be selected for solid block click";
			}
		}
	}

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testHoleRim3D_thinWall) {
	// 1-voxel-thin hollow square tube along Z (wall thickness = 1).
	// Every wall voxel is a surface voxel (air on both sides).
	// BFS must not pick short wall-surface cycles (2x2 patches) as the rim.
	// The correct rim is the 8-voxel ring at Z=5 around the interior 1x1 air column.
	//
	// Layout (XY cross-section, all Z slices):
	//   . . . . .
	//   . W W W .
	//   . W . W .
	//   . W W W .
	//   . . . . .
	// W = solid wall voxel (X in {1,2,3}, Y in {1,2,3}, excluding interior (2,2))
	// Interior air column: (2,2,Z) for all Z

	voxel::RawVolume volume({0, 9});
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int z = 2; z <= 7; ++z) {
		for (int y = 1; y <= 3; ++y) {
			for (int x = 1; x <= 3; ++x) {
				if (x == 2 && y == 2) {
					continue; // interior air
				}
				volume.setVoxel(x, y, z, solid);
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	LUASelectionMode luaMode(_testApp->filesystem());
	ASSERT_TRUE(luaMode.init());
	ASSERT_TRUE(luaMode.loadScript("holerim3d"));

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setLuaSelectionMode(0, &luaMode);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.gridResolution = 1;
	// Click +Z face of a rim voxel at Z=5, air seed at (2,2,5) inside air column
	ctx.cursorFace = voxel::FaceNames::PositiveZ;
	ctx.cursorPosition = glm::ivec3(2, 1, 5);

	EXPECT_TRUE(brush.beginBrush(ctx));
	executeSelect(brush, node, ctx);

	// The 8 rim voxels at Z=5 should be selected; all others not
	for (int z = 2; z <= 7; ++z) {
		for (int y = 1; y <= 3; ++y) {
			for (int x = 1; x <= 3; ++x) {
				if (x == 2 && y == 2) {
					continue; // air
				}
				const bool onRim = (z == 5);
				const int flags = volume.voxel(x, y, z).getFlags() & voxel::FlagOutline;
				if (onRim) {
					EXPECT_NE(flags, 0) << "Rim voxel at (" << x << "," << y << "," << z << ") should be selected";
				} else {
					EXPECT_EQ(flags, 0) << "Non-rim voxel at (" << x << "," << y << "," << z << ") should not be selected";
				}
			}
		}
	}

	brush.shutdown();
	luaMode.shutdown();
}

TEST_F(SelectBrushTest, testSelectModePaint) {
	voxel::RawVolume volume({-5, 5});
	for (int z = -3; z <= 3; ++z) {
		for (int y = -3; y <= 3; ++y) {
			for (int x = -3; x <= 3; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Paint);
	brush.setRadius(2);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	EXPECT_TRUE(brush.beginBrush(ctx));
	EXPECT_TRUE(brush.active());

	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);
	brush.preExecute(ctx, wrapper.volume());
	brush.execute(sceneGraph, wrapper, ctx);

	// Voxels within radius 2 sphere should be selected
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Center voxel should be selected";
	EXPECT_TRUE((volume.voxel(1, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel at distance 1 should be selected";
	EXPECT_TRUE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel at distance 2 should be selected";

	// Voxels outside radius should not be selected
	EXPECT_FALSE((volume.voxel(2, 2, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel at distance ~2.83 should not be selected";
	EXPECT_FALSE((volume.voxel(3, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel at distance 3 should not be selected";

	brush.endBrush(ctx);

	// After endBrush, pending undo region should be valid
	voxel::Region pendingRegion = brush.consumePendingUndoRegion();
	EXPECT_TRUE(pendingRegion.isValid()) << "Paint mode should produce a pending undo region";

	// Second consume should return invalid
	pendingRegion = brush.consumePendingUndoRegion();
	EXPECT_FALSE(pendingRegion.isValid());

	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModePaintDeselect) {
	voxel::RawVolume volume({-5, 5});
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
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Paint);
	brush.setRadius(1);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	EXPECT_TRUE(brush.beginBrush(ctx));

	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, ModifierType::Erase);
	brush.preExecute(ctx, wrapper.volume());
	brush.execute(sceneGraph, wrapper, ctx);

	// Center voxel should be deselected
	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Center voxel should be deselected";
	// Voxel outside radius should still be selected
	EXPECT_TRUE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel at distance 2 should still be selected";

	brush.endBrush(ctx);
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModePaintGrowRegion) {
	voxel::RawVolume volume({-5, 5});
	for (int z = -3; z <= 3; ++z) {
		for (int y = -3; y <= 3; ++y) {
			for (int x = -3; x <= 3; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	// Pre-select a single voxel at origin
	{
		voxel::Voxel v = volume.voxel(0, 0, 0);
		v.setFlags(voxel::FlagOutline);
		volume.setVoxel(0, 0, 0, v);
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Paint);
	brush.setRadius(2);
	brush.setPaintGrowRegion(true);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	EXPECT_TRUE(brush.beginBrush(ctx));

	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);
	brush.preExecute(ctx, wrapper.volume());
	brush.execute(sceneGraph, wrapper, ctx);

	// Direct face-neighbors of origin should be selected (adjacent to pre-selected voxel)
	EXPECT_TRUE((volume.voxel(1, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Face-neighbor should be selected via grow";
	EXPECT_TRUE((volume.voxel(-1, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Face-neighbor should be selected via grow";

	// Diagonal voxel at distance 1.73 has no face-adjacent selected neighbor initially
	// but after its face-neighbors get selected in the same pass it might still not be selected
	// because the visitor reads pre-existing flags. (1,1,0) has face-neighbors (0,1,0) and (1,0,0)
	// - (1,0,0) gets selected in this pass but flags are written live, so it depends on visit order.
	// The important invariant: voxels far from the seed with no selected neighbor chain are NOT selected.
	EXPECT_FALSE((volume.voxel(2, 2, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel far from seed with no adjacent selection should not be selected";

	brush.endBrush(ctx);
	brush.shutdown();
}

TEST_F(SelectBrushTest, testSelectModePaintGrowRegionNoSelection) {
	voxel::RawVolume volume({-5, 5});
	for (int z = -2; z <= 2; ++z) {
		for (int y = -2; y <= 2; ++y) {
			for (int x = -2; x <= 2; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	// No pre-selected voxels
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);

	SelectBrush brush(nullptr);
	ASSERT_TRUE(brush.init());
	brush.setSelectMode(SelectMode::Paint);
	brush.setRadius(2);
	brush.setPaintGrowRegion(true);

	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.cursorFace = voxel::FaceNames::PositiveX;

	EXPECT_TRUE(brush.beginBrush(ctx));

	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);
	brush.preExecute(ctx, wrapper.volume());
	brush.execute(sceneGraph, wrapper, ctx);

	// With grow region enabled but no existing selection, the first stroke selects freely
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "First paint stroke should select freely when no prior selection exists";
	EXPECT_TRUE((volume.voxel(1, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "First paint stroke should select freely when no prior selection exists";

	brush.endBrush(ctx);
	brush.shutdown();
}

} // namespace voxedit
