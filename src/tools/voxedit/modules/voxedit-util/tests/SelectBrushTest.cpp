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

	void executeSelect(SelectBrush &brush, scenegraph::SceneGraphNode &node, const BrushContext &ctx,
					   ModifierType modifierType = ModifierType::Override) {
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, modifierType);
		brush.preExecute(ctx, wrapper.volume());
		brush.execute(sceneGraph, wrapper, ctx);
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

	// Select the entire volume region which includes surface voxels
	prepare(brush, ctx, glm::ivec3(-2, -2, -2), glm::ivec3(2, 2, 2));
	executeSelect(brush, node, ctx, ModifierType::Override);

	// SelectMode::All uses VisitVisible, so only surface voxels should be selected
	// Interior voxel should NOT be selected
	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Interior voxel at (0,0,0) should not be selected";

	// Surface voxels at the actual boundary of the volume should be selected
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

	// SelectMode::All uses VisitVisible, so only visible surface voxels should be deselected
	// Surface voxels at the actual boundary should be deselected
	EXPECT_FALSE((volume.voxel(2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (2,0,0) should be deselected";
	EXPECT_FALSE((volume.voxel(-2, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (-2,0,0) should be deselected";

	// Interior voxel should still be selected (not visited by VisitVisible)
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Interior voxel at (0,0,0) should still be selected";

	brush.shutdown();
}

} // namespace voxedit
