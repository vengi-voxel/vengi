/**
 * @file
 */

#include "../modifier/Modifier.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const ivec3 &v) {
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}
} // namespace glm

namespace voxedit {

class ModifierTest : public app::AbstractTest {
protected:
	void prepare(Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs, ModifierType modifierType, BrushType brushType) {
		modifier.setBrushType(brushType);
		modifier.setModifierType(modifierType);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setGridResolution(1);
		modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX); // mins for aabb
		EXPECT_TRUE(modifier.start());
		if (modifierType != ModifierType::Select && modifierType != ModifierType::ColorPicker) {
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

	void select(voxel::RawVolume &volume, Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		prepare(modifier, mins, maxs, ModifierType::Select, BrushType::None);
		int executed = 0;
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&volume, false);
		EXPECT_TRUE(
			modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType type, bool markUndo) {
				EXPECT_EQ(ModifierType::Select, type) << (int)type;
				EXPECT_EQ(voxel::Region(mins, maxs), region);
				++executed;
			}));
		modifier.stop();
		EXPECT_EQ(1, executed);
	}
};

TEST_F(ModifierTest, testModifierAction) {
	SceneManager mgr(core::make_shared<core::TimeProvider>(), core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr);
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place, BrushType::Shape);
	const voxel::Region region(-10, 10);
	voxel::RawVolume volume(region);
	bool modifierExecuted = false;
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	EXPECT_TRUE(
		modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType modifierType, bool markUndo) {
			modifierExecuted = true;
			EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
		}));
	EXPECT_TRUE(modifierExecuted);
	modifier.shutdown();
}

TEST_F(ModifierTest, testModifierSelection) {
	const voxel::Region region(-10, 10);
	voxel::RawVolume volume(region);

	SceneManager mgr(core::make_shared<core::TimeProvider>(), core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr);
	ASSERT_TRUE(modifier.init());
	select(volume, modifier, glm::ivec3(-1), glm::ivec3(1));

	prepare(modifier, glm::ivec3(-3), glm::ivec3(3), ModifierType::Place, BrushType::Shape);
	int modifierExecuted = 0;
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	EXPECT_TRUE(
		modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType modifierType, bool markUndo) {
			++modifierExecuted;
			EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
		}));
	EXPECT_EQ(1, modifierExecuted);
	modifier.shutdown();
}

} // namespace voxedit
