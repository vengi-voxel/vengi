/**
 * @file
 */

#include "../modifier/Modifier.h"
#include "app/tests/AbstractTest.h"
#include "core/SharedPtr.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"
#include "math/tests/TestMathHelper.h"

namespace voxedit {

class ModifierTest : public app::AbstractTest {
protected:
	void prepare(Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs, ModifierType modifierType,
				 BrushType brushType) {
		modifier.setBrushType(brushType);
		modifier.setModifierType(modifierType);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setGridResolution(1);
		modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX); // mins for aabb
		EXPECT_TRUE(modifier.beginBrush());
		if (brushType == BrushType::Shape) {
			if (modifier.shapeBrush().singleMode()) {
				EXPECT_FALSE(modifier.shapeBrush().active())
					<< "ShapeBrush is active in single mode for modifierType " << (int)modifierType;
				return;
			}
			EXPECT_TRUE(modifier.shapeBrush().active())
				<< "ShapeBrush is not active for modifierType " << (int)modifierType;
		}
		modifier.setCursorPosition(maxs, voxel::FaceNames::PositiveX); // maxs for aabb
		modifier.executeAdditionalAction();
	}

	void select(scenegraph::SceneGraphNode &node, Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		prepare(modifier, mins, maxs, ModifierType::Select, BrushType::Select);
		scenegraph::SceneGraph sceneGraph;
		modifier.execute(sceneGraph, node);
		modifier.endBrush();
	}
};

TEST_F(ModifierTest, testModifierAction) {
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr);
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place, BrushType::Shape);
	voxel::RawVolume volume({-10, 10});
	bool modifierExecuted = false;
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	EXPECT_TRUE(
		modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType modifierType, SceneModifiedFlags flags) {
			modifierExecuted = true;
			EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
		}));
	EXPECT_TRUE(modifierExecuted);
	modifier.shutdown();
}

TEST_F(ModifierTest, testModifierSelection) {
	voxel::RawVolume volume({-10, 10});
	// Fill volume with voxels in the selection area
	for (int z = -1; z <= 1; ++z) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr);
	ASSERT_TRUE(modifier.init());
	select(node, modifier, glm::ivec3(-1), glm::ivec3(1));

	// Verify selection was set correctly using FlagOutline
	EXPECT_TRUE(node.hasSelection()) << "Node should have selection after select()";
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0) << "Center voxel should be selected";
	EXPECT_FALSE((volume.voxel(2, 2, 2).getFlags() & voxel::FlagOutline) != 0) << "Voxel outside selection should not be selected";

	// now modify voxels - but only on the current selection
	// Use Override mode since we already have voxels in the selection area
	prepare(modifier, glm::ivec3(-3), glm::ivec3(3), ModifierType::Override, BrushType::Shape);
	scenegraph::SceneGraph sceneGraph;
	int modifierExecuted = 0;
	EXPECT_TRUE(
		modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType modifierType, SceneModifiedFlags flags) {
			++modifierExecuted;
			EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
		}));
	EXPECT_EQ(1, modifierExecuted);
	EXPECT_TRUE((volume.voxel(-1, -1, -1).getFlags() & voxel::FlagOutline) != 0);
	EXPECT_TRUE((volume.voxel(1, 1, 1).getFlags() & voxel::FlagOutline) != 0);
	EXPECT_FALSE((volume.voxel(2, 2, 2).getFlags() & voxel::FlagOutline) != 0);
	EXPECT_FALSE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(-2, -2, -2).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 2, 2).getMaterial()));
	modifier.shutdown();
}

TEST_F(ModifierTest, testClamp) {
	scenegraph::SceneGraph sceneGraph;
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr);
	ASSERT_TRUE(modifier.init());

	voxel::RawVolume volume(voxel::Region(0, 0, 0, 10, 20, 4));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	TextBrush &brush = modifier.textBrush();
	brush.setInput("ABC");
	brush.setFont("font.ttf");

	modifier.setBrushType(BrushType::Text);
	modifier.setModifierType(ModifierType::Place);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setGridResolution(1);
	modifier.setCursorPosition(volume.region().getLowerCenter(), voxel::FaceNames::PositiveX); // mins for aabb

	{
		brush.setBrushClamping(false);
		voxel::Region dirtyRegion;
		EXPECT_TRUE(modifier.execute(
			sceneGraph, node,
			[&dirtyRegion](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) { dirtyRegion = region; }));
		EXPECT_EQ(dirtyRegion.getDimensionsInVoxels(), glm::ivec3(6, 9, 1));
	}
	volume.clear();
	{
		brush.setBrushClamping(true);
		voxel::Region dirtyRegion;
		EXPECT_TRUE(modifier.execute(
			sceneGraph, node,
			[&dirtyRegion](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) { dirtyRegion = region; }));
		EXPECT_EQ(dirtyRegion.getDimensionsInVoxels(), glm::ivec3(10, 9, 1));
	}

	brush.shutdown();
}

} // namespace voxedit
