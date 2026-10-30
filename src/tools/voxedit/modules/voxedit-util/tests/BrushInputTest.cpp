/**
 * @file
 */

#include "AbstractSceneManagerTest.h"
#include "command/CommandHandler.h"
#include "core/BindingContext.h"
#include "math/Axis.h"
#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/PaintBrush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class BrushInputTest : public AbstractSceneManagerTest {
protected:
	void lockTwoModels(int &firstId, int &secondId) {
		ASSERT_TRUE(_sceneMgr->newScene(true, "session", voxel::Region(0, 0, 0, 8, 2, 2)));
		firstId = _sceneMgr->activeNode();
		secondId = _sceneMgr->addModelChild("second", 9, 3, 3);
		ASSERT_NE(InvalidNodeId, secondId);
		ASSERT_TRUE(_sceneMgr->nodeSetLocked(_sceneMgr->sceneGraph().uuid(firstId), true));
		ASSERT_TRUE(_sceneMgr->nodeSetLocked(_sceneMgr->sceneGraph().uuid(secondId), true));
		ASSERT_TRUE(_sceneMgr->nodeActivate(_sceneMgr->sceneGraph().uuid(firstId)));
	}

	voxel::RawVolume *volumeOf(int nodeId) {
		return _sceneMgr->volume(_sceneMgr->sceneGraph().uuid(nodeId));
	}

	void fillGround(int nodeId, uint8_t color) {
		voxel::RawVolume *volume = volumeOf(nodeId);
		ASSERT_NE(nullptr, volume);
		const voxel::Region &region = volume->region();
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				volume->setVoxel(x, region.getLowerY(), z, voxel::Voxel(voxel::VoxelType::Generic, color));
			}
		}
	}

	void strokeShape(const glm::ivec3 &from, const glm::ivec3 &to) {
		Modifier &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::Shape);
		modifier.currentAABBBrush()->setStrokeMode();
		modifier.setModifierType(ModifierType::Override);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setCursorPosition(from, voxel::FaceNames::PositiveY);
		core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
		command::executeCommands("+actionexecute 1 0.0");
		modifier.setCursorPosition(to, voxel::FaceNames::PositiveY);
		modifier.update(1.0, nullptr);
		command::executeCommands("-actionexecute 1 1.0");
	}
};

TEST_F(BrushInputTest, testHoverPreviewDoesNotChangeCommittedStroke) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "preview", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	ASSERT_TRUE(modifier.currentAABBBrush() != nullptr);
	modifier.currentAABBBrush()->setStrokeMode();
	modifier.setModifierType(ModifierType::Override);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	scenegraph::SceneGraphNode preview(scenegraph::SceneGraphNodeType::Model);
	preview.setVolume(new voxel::RawVolume(*node->volume()));
	preview.setPalette(node->palette());
	modifier.executeBrush(_sceneMgr->sceneGraph(), preview, modifier.modifierType(), modifier.cursorVoxel(), {}, true);
	modifier.setCursorPosition(glm::ivec3(3, 0, 0), voxel::FaceNames::PositiveY);
	modifier.update(1.0, nullptr);
	command::executeCommands("-actionexecute 1 1.0");

	voxel::RawVolume *volume = volumeOf(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, volume);
	for (int x = 0; x <= 3; ++x) {
		EXPECT_FALSE(voxel::isAir(volume->voxel(x, 0, 0).getMaterial())) << x;
	}
	for (int x = 4; x <= 8; ++x) {
		EXPECT_TRUE(voxel::isAir(volume->voxel(x, 0, 0).getMaterial())) << x;
	}
}

TEST_F(BrushInputTest, testPersistentSettingsSurviveEdit) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "settings", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	AABBBrush *b = modifier.currentAABBBrush();
	ASSERT_TRUE(b != nullptr);
	b->setStrokeMode();
	b->setRadius(2);
	b->setMirrorAxis(math::Axis::X, glm::ivec3(4, 0, 0));
	modifier.shapeBrush().setThickness(3);
	strokeShape(glm::ivec3(0), glm::ivec3(1, 0, 0));
	EXPECT_TRUE(b->strokeMode());
	EXPECT_EQ(2, b->radius());
	EXPECT_EQ(math::Axis::X, b->mirrorAxis());
	EXPECT_EQ(3, modifier.shapeBrush().thickness());
	EXPECT_FALSE(modifier.isEditActive());
}

TEST_F(BrushInputTest, testHiddenTargetIsNotWritten) {
	int firstId = InvalidNodeId;
	int secondId = InvalidNodeId;
	lockTwoModels(firstId, secondId);
	ASSERT_TRUE(_sceneMgr->nodeSetVisible(_sceneMgr->sceneGraph().uuid(secondId), false));
	strokeShape(glm::ivec3(0), glm::ivec3(2, 0, 0));
	for (int x = 0; x <= 2; ++x) {
		EXPECT_FALSE(voxel::isAir(volumeOf(firstId)->voxel(x, 0, 0).getMaterial())) << x;
		EXPECT_TRUE(voxel::isAir(volumeOf(secondId)->voxel(x, 0, 0).getMaterial())) << x;
	}
}

TEST_F(BrushInputTest, testSliceGizmoPressRetainsInputWithoutStartingBrushEdit) {
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setSliceMode(true);
	modifier.setBrushGizmoActive(true);
	const size_t history = _sceneMgr->mementoHandler().stateSize();
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	EXPECT_TRUE(modifier.sliceActionPressed());
	EXPECT_FALSE(modifier.isEditActive());
	command::executeCommands("-actionexecute 1 0.1");
	EXPECT_FALSE(modifier.sliceActionPressed());
	EXPECT_FALSE(modifier.isEditActive());
	EXPECT_EQ(_sceneMgr->mementoHandler().stateSize(), history);
	{
		core::ScopedBindingContext sceneContext(core::BindingContext::Context1);
		command::executeCommands("+actionexecute 1 0.2");
		EXPECT_FALSE(modifier.sliceActionPressed());
		command::executeCommands("-actionexecute 1 0.3");
	}
	modifier.setBrushGizmoActive(false);
}

TEST_F(BrushInputTest, testSliceStrokeWritesModelPlane) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "slice_write", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	AABBBrush *b = m.currentAABBBrush();
	ASSERT_TRUE(b != nullptr);
	b->setStrokeMode();
	m.setModifierType(ModifierType::Override);
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setSliceMode(true);
	m.setSliceRegion(voxel::Region(0, 0, 0, 8, 0, 2));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	command::executeCommands("-actionexecute 1 0.05");
	voxel::RawVolume *v = volumeOf(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, v);
	EXPECT_FALSE(voxel::isAir(v->voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(v->voxel(0, 1, 0).getMaterial()));
}

TEST_F(BrushInputTest, testReleaseAppliesUnsampledEndpoint) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "release", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	AABBBrush *b = m.currentAABBBrush();
	ASSERT_NE(nullptr, b);
	b->setStrokeMode();
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	m.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	command::executeCommands("-actionexecute 1 0.05");
	voxel::RawVolume *v = volumeOf(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, v);
	EXPECT_FALSE(voxel::isAir(v->voxel(4, 0, 0).getMaterial()));
}

TEST_F(BrushInputTest, testTemporaryEraseSamplesWhileHeld) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "erase", voxel::Region(0, 0, 0, 8, 2, 2)));
	strokeShape(glm::ivec3(0), glm::ivec3(4, 0, 0));
	Modifier &m = _sceneMgr->modifier();
	m.setModifierType(ModifierType::Place);
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecutedelete 2 2.0");
	m.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	m.update(3.0, nullptr);
	voxel::RawVolume *v = volumeOf(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, v);
	for (int x = 0; x <= 4; ++x) {
		EXPECT_TRUE(voxel::isAir(v->voxel(x, 0, 0).getMaterial())) << x;
	}
	command::executeCommands("-actionexecutedelete 2 3.05");
	EXPECT_EQ(ModifierType::Place, m.modifierType());
	ASSERT_TRUE(_sceneMgr->undo());
	for (int x = 0; x <= 4; ++x) {
		EXPECT_FALSE(voxel::isAir(v->voxel(x, 0, 0).getMaterial())) << x;
	}
}

TEST_F(BrushInputTest, testBoxAppliesOnceOnRelease) {
	int firstId = InvalidNodeId;
	int secondId = InvalidNodeId;
	lockTwoModels(firstId, secondId);
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	AABBBrush *b = m.currentAABBBrush();
	ASSERT_NE(nullptr, b);
	b->setBoxMode();
	m.setModifierType(ModifierType::Override);
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	ASSERT_TRUE(m.isEditActive());
	ASSERT_NE(nullptr, volumeOf(firstId));
	ASSERT_NE(nullptr, volumeOf(secondId));
	EXPECT_TRUE(voxel::isAir(volumeOf(firstId)->voxel(0, 0, 0).getMaterial()));
	m.setCursorPosition(glm::ivec3(2, 2, 2), voxel::FaceNames::PositiveY);
	command::executeCommands("-actionexecute 1 0.05");
	EXPECT_FALSE(m.isEditActive());
	for (int id : {firstId, secondId}) {
		voxel::RawVolume *v = volumeOf(id);
		ASSERT_NE(nullptr, v);
		EXPECT_FALSE(voxel::isAir(v->voxel(0, 0, 0).getMaterial())) << id;
		EXPECT_FALSE(voxel::isAir(v->voxel(2, 2, 2).getMaterial())) << id;
		EXPECT_FALSE(voxel::isAir(v->voxel(1, 1, 1).getMaterial())) << id;
	}
	ASSERT_TRUE(_sceneMgr->undo());
	for (int id : {firstId, secondId}) {
		voxel::RawVolume *v = volumeOf(id);
		ASSERT_NE(nullptr, v);
		EXPECT_TRUE(voxel::isAir(v->voxel(1, 1, 1).getMaterial())) << id;
	}
}

TEST_F(BrushInputTest, testBoxWaitsForDepthThenApplies) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "box_depth", voxel::Region(0, 0, 0, 8, 4, 8)));
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	AABBBrush *b = m.currentAABBBrush();
	ASSERT_NE(nullptr, b);
	b->setBoxMode();
	m.setModifierType(ModifierType::Override);
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	ASSERT_TRUE(m.isEditActive());
	m.setCursorPosition(glm::ivec3(2, 0, 2), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(m.needsAdditionalAction());
	command::executeCommands("-actionexecute 1 0.05");
	ASSERT_TRUE(m.isEditActive());
	voxel::RawVolume *v = volumeOf(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, v);
	EXPECT_TRUE(voxel::isAir(v->voxel(0, 0, 0).getMaterial()));
	m.setCursorPosition(glm::ivec3(2, 3, 2), voxel::FaceNames::PositiveY);
	command::executeCommands("+actionexecute 1 0.1");
	command::executeCommands("-actionexecute 1 0.15");
	EXPECT_FALSE(m.isEditActive());
	EXPECT_FALSE(voxel::isAir(v->voxel(0, 0, 0).getMaterial()));
	EXPECT_FALSE(voxel::isAir(v->voxel(2, 3, 2).getMaterial()));
}

TEST_F(BrushInputTest, testCenterAppliesOnRelease) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "center", voxel::Region(0, 0, 0, 8, 4, 8)));
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	AABBBrush *b = m.currentAABBBrush();
	ASSERT_NE(nullptr, b);
	b->setCenterMode();
	m.setModifierType(ModifierType::Override);
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(4, 2, 4), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	m.setCursorPosition(glm::ivec3(6, 4, 6), voxel::FaceNames::PositiveY);
	command::executeCommands("-actionexecute 1 0.05");
	EXPECT_FALSE(m.isEditActive());
	voxel::RawVolume *v = volumeOf(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, v);
	EXPECT_FALSE(voxel::isAir(v->voxel(4, 2, 4).getMaterial()));
	EXPECT_FALSE(voxel::isAir(v->voxel(2, 0, 2).getMaterial()));
	EXPECT_FALSE(voxel::isAir(v->voxel(6, 4, 6).getMaterial()));
}

TEST_F(BrushInputTest, testPaintBoxAppliesExistingVoxels) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "paint_box", voxel::Region(0, 0, 0, 8, 2, 2)));
	const int nodeId = _sceneMgr->activeNode();
	fillGround(nodeId, 0);
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Paint);
	AABBBrush *b = m.currentAABBBrush();
	ASSERT_NE(nullptr, b);
	b->setBoxMode();
	m.paintBrush().setPaintMode(PaintBrush::PaintMode::Replace);
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	m.setCursorPosition(glm::ivec3(2, 2, 2), voxel::FaceNames::PositiveY);
	command::executeCommands("-actionexecute 1 0.05");
	EXPECT_FALSE(m.isEditActive());
	voxel::RawVolume *v = volumeOf(nodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(1, v->voxel(0, 0, 0).getColor());
	EXPECT_EQ(1, v->voxel(2, 0, 2).getColor());
	EXPECT_EQ(0, v->voxel(3, 0, 0).getColor());
}

TEST_F(BrushInputTest, testPlaneReleaseResetsTraceForNextClick) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "plane_again", voxel::Region(0, 0, 0, 8, 4, 8)));
	const int nodeId = _sceneMgr->activeNode();
	fillGround(nodeId, 1);
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Plane);
	m.setModifierType(ModifierType::Place);
	const voxel::Voxel placed = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	m.setCursorVoxel(placed);
	m.setHitCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(1, 1, 1), voxel::FaceNames::PositiveY);
	sceneMgr()->testSetLastRaytrace(12, 34);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	ASSERT_TRUE(m.isEditActive());
	EXPECT_FALSE(m.needsAdditionalAction());
	command::executeCommands("-actionexecute 1 0.05");
	EXPECT_FALSE(m.isEditActive());
	voxel::RawVolume *v = volumeOf(nodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(2, v->voxel(1, 1, 1).getColor());
	EXPECT_TRUE(voxel::isAir(v->voxel(1, 2, 1).getMaterial()));
	EXPECT_EQ(-1, sceneMgr()->testLastRaytraceX());
	EXPECT_EQ(-1, sceneMgr()->testLastRaytraceY());

	m.setHitCursorVoxel(v->voxel(1, 1, 1));
	m.setCursorPosition(glm::ivec3(1, 2, 1), voxel::FaceNames::PositiveY);
	command::executeCommands("+actionexecute 1 0.1");
	ASSERT_TRUE(m.isEditActive());
	command::executeCommands("-actionexecute 1 0.15");
	EXPECT_FALSE(m.isEditActive());
	EXPECT_EQ(2, v->voxel(1, 2, 1).getColor());
}

} // namespace voxedit
