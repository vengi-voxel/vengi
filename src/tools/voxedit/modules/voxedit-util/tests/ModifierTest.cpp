/**
 * @file
 */

#include "../modifier/Modifier.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxedit {

class ModifierTest : public app::AbstractTest {
protected:
	void prepare(Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs, ModifierType modifierType) {
		modifier.setBrushType(BrushType::Shape);
		modifier.setModifierType(modifierType);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setGridResolution(1);
		modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX); // mins for aabb
		modifier.setBrushType(BrushType::Shape);
		EXPECT_TRUE(modifier.start());
		if (modifier.shapeBrush().singleMode()) {
			EXPECT_FALSE(modifier.activeShapeBrush()->active());
		} else {
			EXPECT_TRUE(modifier.activeShapeBrush()->active());
			modifier.setCursorPosition(maxs, voxel::FaceNames::PositiveX); // maxs for aabb
			modifier.executeAdditionalAction();
		}
	}

	void select(voxel::RawVolume &volume, Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		prepare(modifier, mins, maxs, ModifierType::Select);
		int executed = 0;
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&volume, false);
		modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType type, bool markUndo) {
			EXPECT_EQ(ModifierType::Select, type);
			EXPECT_EQ(voxel::Region(mins, maxs), region);
			++executed;
		});
		modifier.stop();
		EXPECT_EQ(1, executed);
	}
};

TEST_F(ModifierTest, testModifierAction) {
	Modifier modifier;
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place);
	const voxel::Region region(-10, 10);
	voxel::RawVolume volume(region);
	bool modifierExecuted = false;
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType modifierType, bool markUndo) {
		modifierExecuted = true;
		EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
	});
	EXPECT_TRUE(modifierExecuted);
	modifier.shutdown();
}

TEST_F(ModifierTest, testModifierSelection) {
	const voxel::Region region(-10, 10);
	voxel::RawVolume volume(region);

	Modifier modifier;
	ASSERT_TRUE(modifier.init());
	select(volume, modifier, glm::ivec3(-1), glm::ivec3(1));

	prepare(modifier, glm::ivec3(-3), glm::ivec3(3), ModifierType::Place);
	int modifierExecuted = 0;
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType modifierType, bool markUndo) {
		++modifierExecuted;
		EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
	});
	EXPECT_EQ(1, modifierExecuted);
	modifier.shutdown();
}

TEST_F(ModifierTest, DISABLED_testPath) {
	// TODO: implement me
}

TEST_F(ModifierTest, DISABLED_testLine) {
	// TODO: implement me
}

} // namespace voxedit
