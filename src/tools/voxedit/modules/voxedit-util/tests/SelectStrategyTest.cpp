/**
 * @file
 */

#include "../modifier/brush/select/All.h"
#include "../modifier/brush/select/Connected.h"
#include "../modifier/brush/select/Lasso.h"
#include "../modifier/brush/select/SameColor.h"
#include "../modifier/brush/select/Surface.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxedit {

class SelectStrategyTest : public app::AbstractTest {
protected:
	scenegraph::SceneGraph _sceneGraph;

	void fillCube(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs, uint8_t color = 1) {
		for (int z = mins.z; z <= maxs.z; ++z) {
			for (int y = mins.y; y <= maxs.y; ++y) {
				for (int x = mins.x; x <= maxs.x; ++x) {
					volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, color));
				}
			}
		}
	}

	int countSelected(const voxel::RawVolume &volume, const voxel::Region &region) {
		// TODO: PERF: use a visitor here instead of iterating manually
		int count = 0;
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					if (volume.voxel(x, y, z).getFlags() & voxel::FlagOutline) {
						++count;
					}
				}
			}
		}
		return count;
	}
};

TEST_F(SelectStrategyTest, testAllStrategy) {
	const voxel::Region region(glm::ivec3(-2), glm::ivec3(2));
	voxel::RawVolume volume(region);
	fillCube(volume, region.getLowerCorner(), region.getUpperCorner());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	select::All strategy;
	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	select::AABBBrushState state;

	strategy.generate(_sceneGraph, wrapper, ctx, volume.region(), state);

	// All 5x5x5 = 125 solid voxels should be selected
	EXPECT_EQ(countSelected(volume, volume.region()), region.voxels());
}

TEST_F(SelectStrategyTest, testAllStrategyErase) {
	const voxel::Region region(glm::ivec3(-2), glm::ivec3(2));
	voxel::RawVolume volume(region);
	fillCube(volume, region.getLowerCorner(), region.getUpperCorner());
	// Pre-select all voxels
	volume.setFlags(volume.region(), voxel::FlagOutline);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Erase);

	select::All strategy;
	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	select::AABBBrushState state;

	strategy.generate(_sceneGraph, wrapper, ctx, volume.region(), state);

	EXPECT_EQ(countSelected(volume, volume.region()), 0);
}

TEST_F(SelectStrategyTest, testSurfaceStrategy) {
	const voxel::Region region(glm::ivec3(-2), glm::ivec3(2));
	voxel::RawVolume volume(region);
	fillCube(volume, region.getLowerCorner(), region.getUpperCorner());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	select::Surface strategy;
	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	select::AABBBrushState state;

	strategy.generate(_sceneGraph, wrapper, ctx, volume.region(), state);

	int selected = countSelected(volume, volume.region());
	// Surface of 5x5x5 cube: 125 - 3x3x3 interior = 125 - 27 = 98
	EXPECT_EQ(selected, 98);
	// Interior voxel should NOT be selected
	EXPECT_FALSE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0);
}

TEST_F(SelectStrategyTest, testSameColorStrategy) {
	const voxel::Region region(glm::ivec3(-2), glm::ivec3(2));
	voxel::RawVolume volume(region);
	// Fill with color 1
	fillCube(volume, region.getLowerCorner(), region.getUpperCorner(), 1);
	// Overwrite some with color 2
	fillCube(volume, glm::ivec3(0), region.getUpperCorner(), 2);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	select::SameColor strategy;
	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.hitCursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	select::AABBBrushState state;

	strategy.generate(_sceneGraph, wrapper, ctx, volume.region(), state);

	// Only color-1 voxels should be selected
	// Color 2 region is 3x3x3 = 27 voxels
	// Total solid = 125, color 1 = 125 - 27 = 98
	EXPECT_EQ(countSelected(volume, volume.region()), 98);
	// A color-2 voxel should NOT be selected
	EXPECT_FALSE((volume.voxel(1, 1, 1).getFlags() & voxel::FlagOutline) != 0);
}

TEST_F(SelectStrategyTest, testConnectedStrategy) {
	const voxel::Region region(glm::ivec3(-5), glm::ivec3(5));
	voxel::RawVolume volume(region);
	// Two disconnected cubes
	fillCube(volume, region.getLowerCorner(), glm::ivec3(-3, -3, -3), 1);
	fillCube(volume, glm::ivec3(3, 3, 3), region.getUpperCorner(), 1);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	select::Connected strategy;
	BrushContext ctx;
	ctx.targetVolumeRegion = volume.region();
	ctx.cursorPosition = glm::ivec3(-4, -4, -4);
	ctx.hitCursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	select::AABBBrushState state;

	strategy.generate(_sceneGraph, wrapper, ctx, volume.region(), state);

	// Only the first cube (3x3x3 = 27 voxels) should be selected
	EXPECT_EQ(countSelected(volume, volume.region()), 27);
	// Second cube should NOT be selected
	EXPECT_FALSE((volume.voxel(4, 4, 4).getFlags() & voxel::FlagOutline) != 0);
}

TEST_F(SelectStrategyTest, testLassoPointInPolygon) {
	// Test the lasso's pointInPolygon indirectly through state management
	select::Lasso strategy(nullptr);

	EXPECT_FALSE(strategy.active());
	EXPECT_TRUE(strategy.screenPoints().empty());

	BrushContext ctx;
	select::AABBBrushState state;

	// Begin starts dragging
	EXPECT_TRUE(strategy.beginBrush(ctx, state));
	EXPECT_TRUE(strategy.active());

	// Second begin while dragging returns true without resetting
	EXPECT_TRUE(strategy.beginBrush(ctx, state));
	EXPECT_TRUE(strategy.active());

	// Abort cleans up
	strategy.abort(ctx);
	EXPECT_FALSE(strategy.active());
	EXPECT_TRUE(strategy.screenPoints().empty());
}

TEST_F(SelectStrategyTest, testLassoGenerateRequiresMinPoints) {
	select::Lasso strategy(nullptr);

	BrushContext ctx;
	select::AABBBrushState state;
	strategy.beginBrush(ctx, state);

	// generate with < 3 points should just clean up without crashing

	const voxel::Region region(glm::ivec3(-2), glm::ivec3(2));
	voxel::RawVolume volume(region);
	fillCube(volume, region.getLowerCorner(), region.getUpperCorner());
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	strategy.generate(_sceneGraph, wrapper, ctx, volume.region(), state);

	// Nothing should be selected (not enough points, no SceneManager)
	EXPECT_EQ(countSelected(volume, volume.region()), 0);
	EXPECT_FALSE(strategy.active());
}

} // namespace voxedit
