/**
 * @file
 */

#include "../modifier/Modifier.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierType.h"
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
	void prepare(Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs, ModifierType modifierType) {
		modifier.setBrushType(BrushType::Shape);
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
		prepare(modifier, mins, maxs, ModifierType::Select);
		int executed = 0;
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&volume, false);
		EXPECT_TRUE(
			modifier.execute(sceneGraph, node, [&](const voxel::Region &region, ModifierType type, bool markUndo) {
				EXPECT_EQ(ModifierType::Select, type);
				EXPECT_EQ(voxel::Region(mins, maxs), region);
				++executed;
			}));
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

	Modifier modifier;
	ASSERT_TRUE(modifier.init());
	select(volume, modifier, glm::ivec3(-1), glm::ivec3(1));

	prepare(modifier, glm::ivec3(-3), glm::ivec3(3), ModifierType::Place);
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

TEST_F(ModifierTest, testPath) {
	const voxel::Region region(-10, 10);
	voxel::RawVolume volume(region);
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	Modifier modifier;
	ASSERT_TRUE(modifier.init());

	const glm::ivec3 mins = region.getLowerCorner();
	const glm::ivec3 maxs(region.getUpperX(), region.getLowerY(), region.getUpperZ());
	prepare(modifier, mins, maxs, ModifierType::Place);
	EXPECT_TRUE(modifier.execute(
		sceneGraph, node, [&](const voxel::Region &modifiedRegion, ModifierType modifierType, bool markUndo) {}));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(mins).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(volume.voxel(maxs).getMaterial()));

	int modifierExecuted = 0;
	modifier.setModifierType(ModifierType::Path);
	const glm::ivec3 referencePosition(region.getLowerX(), region.getLowerY() + 1, region.getLowerZ());
	modifier.setReferencePosition(referencePosition);
	const glm::ivec3 cursorPosition(region.getUpperX(), region.getLowerY() + 1, region.getUpperZ());
	modifier.setCursorPosition(cursorPosition, voxel::FaceNames::PositiveY);
	EXPECT_TRUE(modifier.execute(sceneGraph, node,
								 [&](const voxel::Region &modifiedRegion, ModifierType modifierType, bool markUndo) {
									 ++modifierExecuted;
									 EXPECT_EQ(referencePosition, modifiedRegion.getLowerCorner());
									 EXPECT_EQ(cursorPosition, modifiedRegion.getUpperCorner());
								 }));
	EXPECT_EQ(1, modifierExecuted);
	modifier.shutdown();
}

TEST_F(ModifierTest, DISABLED_testLine) {
	// TODO: implement me
}

} // namespace voxedit
