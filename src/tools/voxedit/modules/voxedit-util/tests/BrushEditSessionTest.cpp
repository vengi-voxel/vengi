/**
 * @file
 */

#include "AbstractSceneManagerTest.h"
#include "color/RGBA.h"
#include "command/CommandHandler.h"
#include "core/BindingContext.h"
#include "math/Axis.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class BrushEditSessionTest : public AbstractSceneManagerTest {
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
		modifier.shapeBrush().setStrokeMode();
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

TEST_F(BrushEditSessionTest, testFastStrokeReachesEveryLockedTarget) {
	int firstId = InvalidNodeId;
	int secondId = InvalidNodeId;
	lockTwoModels(firstId, secondId);
	strokeShape(glm::ivec3(0), glm::ivec3(4, 0, 0));

	voxel::RawVolume *first = volumeOf(firstId);
	voxel::RawVolume *second = volumeOf(secondId);
	ASSERT_NE(nullptr, first);
	ASSERT_NE(nullptr, second);
	for (int x = 0; x <= 4; ++x) {
		EXPECT_FALSE(voxel::isAir(first->voxel(x, 0, 0).getMaterial())) << "first node missing dab " << x;
		EXPECT_FALSE(voxel::isAir(second->voxel(x, 0, 0).getMaterial())) << "second node missing dab " << x;
	}
	EXPECT_FALSE(_sceneMgr->modifier().isEditActive());
}

TEST_F(BrushEditSessionTest, testBlendKeepsPerTargetOriginals) {
	int firstId = InvalidNodeId;
	int secondId = InvalidNodeId;
	lockTwoModels(firstId, secondId);

	scenegraph::SceneGraphNode &firstNode = _sceneMgr->sceneGraph().node(_sceneMgr->sceneGraph().uuid(firstId));
	scenegraph::SceneGraphNode &secondNode = _sceneMgr->sceneGraph().node(_sceneMgr->sceneGraph().uuid(secondId));
	firstNode.palette().setColor(0, color::RGBA(0, 0, 0));
	firstNode.palette().setColor(1, color::RGBA(255, 255, 255));
	secondNode.palette().setColor(0, color::RGBA(0, 0, 0));
	secondNode.palette().setColor(2, color::RGBA(0, 0, 255));
	secondNode.palette().setColor(1, color::RGBA(255, 255, 255));
	fillGround(firstId, 0);
	fillGround(secondId, 2);

	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.paintBrush().setPaintMode(PaintBrush::PaintMode::Blend);
	modifier.paintBrush().setBlendOpacity(0.5f);
	modifier.paintBrush().setRadius(0);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0, 0, 0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	command::executeCommands("-actionexecute 1 1.0");

	const uint8_t firstColor = volumeOf(firstId)->voxel(0, 0, 0).getColor();
	const uint8_t secondColor = volumeOf(secondId)->voxel(0, 0, 0).getColor();
	EXPECT_NE(0, (int)firstColor) << "First target should leave its black original";
	EXPECT_NE(2, (int)secondColor) << "Second target should leave its blue original";
	EXPECT_NE((int)firstColor, (int)secondColor) << "Independent originals must not share blend bookkeeping";
}

TEST_F(BrushEditSessionTest, testHistoryRecordsPerTargetBounds) {
	int firstId = InvalidNodeId;
	int secondId = InvalidNodeId;
	lockTwoModels(firstId, secondId);

	voxel::RawVolume *second = volumeOf(secondId);
	ASSERT_NE(nullptr, second);
	second->setVoxel(0, 0, 0, voxel::Voxel(voxel::VoxelType::Generic, 3));

	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.paintBrush().setPaintMode(PaintBrush::PaintMode::Replace);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	modifier.setCursorPosition(glm::ivec3(3, 0, 0), voxel::FaceNames::PositiveY);
	modifier.update(1.0, nullptr);
	command::executeCommands("-actionexecute 1 1.0");

	const memento::MementoStateGroup &group = _sceneMgr->mementoHandler().stateGroup();
	int firstStates = 0;
	int secondStates = 0;
	for (const memento::MementoState &state : group.states) {
		if (state.nodeUUID == _sceneMgr->sceneGraph().uuid(firstId)) {
			++firstStates;
			EXPECT_FALSE(state.dataRegion().isValid()) << "Unchanged first target must not get a history region";
		}
		if (state.nodeUUID == _sceneMgr->sceneGraph().uuid(secondId)) {
			++secondStates;
			EXPECT_EQ(state.dataRegion(), voxel::Region(0, 0, 0, 0, 0, 0));
		}
	}
	EXPECT_EQ(0, firstStates);
	EXPECT_EQ(1, secondStates);
}

TEST_F(BrushEditSessionTest, testOneUndoRestoresAllTargets) {
	int firstId = InvalidNodeId;
	int secondId = InvalidNodeId;
	lockTwoModels(firstId, secondId);
	strokeShape(glm::ivec3(0), glm::ivec3(3, 0, 0));

	ASSERT_TRUE(_sceneMgr->mementoHandler().canUndo());
	ASSERT_TRUE(_sceneMgr->undo());
	for (int x = 0; x <= 3; ++x) {
		EXPECT_TRUE(voxel::isAir(volumeOf(firstId)->voxel(x, 0, 0).getMaterial())) << x;
		EXPECT_TRUE(voxel::isAir(volumeOf(secondId)->voxel(x, 0, 0).getMaterial())) << x;
	}
	ASSERT_TRUE(_sceneMgr->mementoHandler().canRedo());
	ASSERT_TRUE(_sceneMgr->redo());
	for (int x = 0; x <= 3; ++x) {
		EXPECT_FALSE(voxel::isAir(volumeOf(firstId)->voxel(x, 0, 0).getMaterial())) << x;
		EXPECT_FALSE(voxel::isAir(volumeOf(secondId)->voxel(x, 0, 0).getMaterial())) << x;
	}
}

TEST_F(BrushEditSessionTest, testUnchangedEditPreservesRedo) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "unchanged", voxel::Region(0, 0, 0, 8, 2, 2)));
	strokeShape(glm::ivec3(0), glm::ivec3(0, 0, 0));
	ASSERT_TRUE(_sceneMgr->undo());
	ASSERT_TRUE(_sceneMgr->mementoHandler().canRedo());
	const size_t states = _sceneMgr->mementoHandler().stateSize();

	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Paint);
	modifier.paintBrush().setStrokeMode();
	modifier.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	command::executeCommands("-actionexecute 1 1.0");

	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
	EXPECT_TRUE(_sceneMgr->mementoHandler().canRedo());
	EXPECT_FALSE(modifier.isEditActive());
}

TEST_F(BrushEditSessionTest, testDuplicateFinishDoesNotReplay) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "dup", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setStrokeMode();
	modifier.setModifierType(ModifierType::Override);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(1, 0, 0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	command::executeCommands("-actionexecute 1 1.0");
	const size_t states = _sceneMgr->mementoHandler().stateSize();
	EXPECT_TRUE(modifier.finishEdit());
	EXPECT_TRUE(modifier.finishEdit());
	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
	EXPECT_FALSE(voxel::isAir(volumeOf(_sceneMgr->activeNode())->voxel(1, 0, 0).getMaterial()));
}

TEST_F(BrushEditSessionTest, testToolChangeFinishesStrokeOnce) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "switch", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setStrokeMode();
	modifier.shapeBrush().setRadius(2);
	modifier.setModifierType(ModifierType::Override);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	EXPECT_TRUE(modifier.isEditActive());
	modifier.setCursorPosition(glm::ivec3(2, 0, 0), voxel::FaceNames::PositiveY);
	modifier.update(1.0, nullptr);
	modifier.setBrushType(BrushType::Paint);
	EXPECT_FALSE(modifier.isEditActive());
	EXPECT_EQ(BrushType::Paint, modifier.brushType());
	EXPECT_FALSE(voxel::isAir(volumeOf(_sceneMgr->activeNode())->voxel(2, 0, 0).getMaterial()));
	ASSERT_TRUE(_sceneMgr->undo());
	EXPECT_TRUE(voxel::isAir(volumeOf(_sceneMgr->activeNode())->voxel(2, 0, 0).getMaterial()));
}

TEST_F(BrushEditSessionTest, testTemporaryEraseDoesNotStick) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "erase", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setStrokeMode();
	modifier.setModifierType(ModifierType::Place);
	modifier.lock();
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecutedelete 1 0.0");
	EXPECT_FALSE(modifier.isEditActive());
	EXPECT_EQ(ModifierType::Place, modifier.modifierType());
	command::executeCommands("-actionexecutedelete 1 1.0");
	modifier.unlock();

	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	command::executeCommands("+actionexecute 1 0.0");
	EXPECT_TRUE(modifier.isEditActive());
	modifier.abort();
	EXPECT_FALSE(modifier.isEditActive());
	EXPECT_EQ(ModifierType::Place, modifier.modifierType());
}

TEST_F(BrushEditSessionTest, testReplacedVolumeIsNotWritten) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "replace", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setStrokeMode();
	modifier.setModifierType(ModifierType::Override);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	ASSERT_TRUE(modifier.beginEdit());
	EXPECT_TRUE(modifier.isEditActive());
	ASSERT_TRUE(modifier.updateEdit());

	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	voxel::RawVolume *replacement = new voxel::RawVolume(node->region());
	node->setVolume(replacement);

	modifier.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	modifier.updateEdit();
	modifier.finishEdit();
	for (int x = 1; x <= 4; ++x) {
		EXPECT_TRUE(voxel::isAir(replacement->voxel(x, 0, 0).getMaterial())) << x;
	}
}

TEST_F(BrushEditSessionTest, testUnchangedViewportRegionKeepsStrokeActive) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setSliceRegion(voxel::Region::InvalidRegion);
	ASSERT_TRUE(m.beginEdit());
	m.setSliceRegion(voxel::Region::InvalidRegion);
	EXPECT_TRUE(m.isEditActive());
	m.finishEdit();
}

TEST_F(BrushEditSessionTest, testViewportClearDoesNotFinishSliceStroke) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "slice_keep", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setModifierType(ModifierType::Override);
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setSliceMode(true);
	m.setSliceRegion(voxel::Region(0, 0, 0, 8, 0, 2));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(m.beginEdit(ModifierType::None, true));
	ASSERT_TRUE(m.updateEdit());
	m.setSliceMode(false);
	m.setSliceRegion(voxel::Region::InvalidRegion);
	EXPECT_TRUE(m.isEditActive());
	m.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	EXPECT_TRUE(m.updateEdit());
	m.finishEdit();
	voxel::RawVolume *volume = volumeOf(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, volume);
	EXPECT_FALSE(voxel::isAir(volume->voxel(0, 0, 0).getMaterial()));
	EXPECT_FALSE(voxel::isAir(volume->voxel(4, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume->voxel(0, 1, 0).getMaterial()));
}

TEST_F(BrushEditSessionTest, testOtherSlicePlaneFinishesEdit) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setSliceRegion(voxel::Region(0, 0, 0, 8, 0, 2));
	ASSERT_TRUE(m.beginEdit(ModifierType::None, true));
	m.setSliceRegion(voxel::Region(0, 1, 0, 8, 1, 2));
	EXPECT_FALSE(m.isEditActive());
}

TEST_F(BrushEditSessionTest, testReleaseAfterToolSwitchDoesNotExecuteNewTool) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "release_tool", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	m.setBrushType(BrushType::Stamp);
	m.stampBrush().setVoxel(m.cursorVoxel(),
										 _sceneMgr->sceneGraph().node(_sceneMgr->activeNode()).palette());
	m.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	command::executeCommands("-actionexecute 1 0.05");
	EXPECT_TRUE(voxel::isAir(volumeOf(_sceneMgr->activeNode())->voxel(4, 0, 0).getMaterial()));
}

TEST_F(BrushEditSessionTest, testReplacementDoesNotGetOldHistory) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(m.beginEdit());
	ASSERT_TRUE(m.updateEdit());
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	voxel::RawVolume *replacement = new voxel::RawVolume(node->region());
	node->setVolume(replacement);
	const size_t states = _sceneMgr->mementoHandler().stateSize();
	m.finishEdit(BrushEditFinishReason::ExternalInvalidate);
	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
}

TEST_F(BrushEditSessionTest, testSliceReplacementDoesNotReceiveOldEdit) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "slice_replace", voxel::Region(0, 0, 0, 8, 2, 2)));
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setSliceMode(true);
	m.setSliceRegion(voxel::Region(0, 0, 0, 8, 0, 2));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(m.beginEdit(ModifierType::None, true));
	ASSERT_TRUE(m.updateEdit());
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	voxel::RawVolume *replacement = new voxel::RawVolume(node->region());
	node->setVolume(replacement);
	m.setCursorPosition(glm::ivec3(4, 0, 0), voxel::FaceNames::PositiveY);
	m.updateEdit();
	EXPECT_TRUE(voxel::isAir(replacement->voxel(4, 0, 0).getMaterial()));
	m.finishEdit();
}

TEST_F(BrushEditSessionTest, testSettingsSurviveToolRoundTrip) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setThickness(3);
	m.shapeBrush().setMirrorAxis(math::Axis::X, glm::ivec3(4, 0, 0));
	m.setBrushType(BrushType::Paint);
	m.setBrushType(BrushType::Shape);
	EXPECT_EQ(3, m.shapeBrush().thickness());
	EXPECT_EQ(math::Axis::X, m.shapeBrush().mirrorAxis());
}

TEST_F(BrushEditSessionTest, testModeChangeFinishesSession) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	ASSERT_TRUE(m.beginEdit());
	command::executeCommands("setshapebrushbox");
	m.updateEdit();
	EXPECT_FALSE(m.isEditActive());
	m.finishEdit();
}

TEST_F(BrushEditSessionTest, testSameColorPaintPreservesRedo) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "noop", voxel::Region(0, 0, 0, 8, 2, 2)));
	strokeShape(glm::ivec3(0), glm::ivec3(0));
	strokeShape(glm::ivec3(1, 0, 0), glm::ivec3(1, 0, 0));
	ASSERT_TRUE(_sceneMgr->undo());
	ASSERT_TRUE(_sceneMgr->mementoHandler().canRedo());
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Paint);
	m.paintBrush().setStrokeMode();
	m.paintBrush().setPaintMode(PaintBrush::PaintMode::Replace);
	m.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	command::executeCommands("-actionexecute 1 0.05");
	EXPECT_TRUE(_sceneMgr->mementoHandler().canRedo());
}

TEST_F(BrushEditSessionTest, testOldReleaseDoesNotFinishNewSession) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	const uint64_t firstId = m.editId();
	command::executeCommands("+actionexecutedelete 2 0.01");
	EXPECT_NE(firstId, m.editId());
	command::executeCommands("-actionexecute 1 0.02");
	EXPECT_TRUE(m.isEditActive());
	EXPECT_EQ(ModifierType::Erase, m.modifierType());
	command::executeCommands("-actionexecutedelete 2 0.03");
	EXPECT_FALSE(m.isEditActive());
	EXPECT_EQ(ModifierType::Place, m.modifierType());
}

TEST_F(BrushEditSessionTest, testSecondBindingDoesNotRestartSession) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	const uint64_t id = m.editId();
	command::executeCommands("+actionexecute 2 0.01");
	EXPECT_EQ(id, m.editId());
	command::executeCommands("-actionexecute 1 0.02");
	EXPECT_TRUE(m.isEditActive());
	command::executeCommands("-actionexecute 2 0.03");
	EXPECT_FALSE(m.isEditActive());
}

TEST_F(BrushEditSessionTest, testSettingsChangesFinishBeforeMutation) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	const core::Function<void()> changes[] = {
		[&]() { m.shapeBrush().setRadius(1); },
		[&]() { m.shapeBrush().setThickness(2); },
		[&]() { command::executeCommands("shapeellipse"); },
		[&]() { m.shapeBrush().setMirrorAxis(math::Axis::X, glm::ivec3(2)); },
		[&]() { m.shapeBrush().setClampToVolume(true); },
		[&]() { m.setGridResolution(2); },
		[&]() { m.setLockedAxis(math::Axis::X, false); },
		[&]() { m.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 2)); }};
	for (const core::Function<void()> &change : changes) {
		ASSERT_TRUE(m.beginEdit());
		change();
		m.updateEdit();
		EXPECT_FALSE(m.isEditActive());
	}
	m.setBrushType(BrushType::Paint);
	const core::Function<void()> paintChanges[] = {
		[&]() { m.paintBrush().setStrength(2.0f); },
		[&]() { m.paintBrush().setBlendOpacity(0.5f); },
		[&]() { m.paintBrush().setVariationChance(4); },
		[&]() { m.paintBrush().setPaintMode(PaintBrush::PaintMode::Blend); },
		[&]() { m.paintBrush().setFloodFill(); }};
	for (const core::Function<void()> &change : paintChanges) {
		ASSERT_TRUE(m.beginEdit());
		change();
		m.updateEdit();
		EXPECT_FALSE(m.isEditActive());
	}
}

TEST_F(BrushEditSessionTest, testUnchangedSettingsKeepSessionActive) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	ASSERT_TRUE(m.beginEdit());
	m.setModifierType(m.modifierType());
	m.shapeBrush().setStrokeMode();
	m.shapeBrush().setRadius(m.shapeBrush().radius());
	m.shapeBrush().setThickness(m.shapeBrush().thickness());
	m.shapeBrush().setClampToVolume(m.shapeBrush().clampToVolume());
	m.setCursorVoxel(m.cursorVoxel());
	m.setGridResolution(1);
	EXPECT_TRUE(m.isEditActive());
	m.finishEdit(BrushEditFinishReason::Interrupt);
}

TEST_F(BrushEditSessionTest, testDirectSettingsChangeFinishesStrokeOnFrameUpdate) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "settings", voxel::Region(0, 8)));
	const core::UUID uuid = _sceneMgr->activeNodeUUID();
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	ShapeBrush &brush = modifier.shapeBrush();
	brush.setStrokeMode();
	brush.setRadius(0);
	modifier.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateEdit());
	brush.setRadius(1);
	modifier.setCursorPosition(glm::ivec3(3, 0, 0), voxel::FaceNames::PositiveY);
	modifier.flushPendingBrushChanges();
	EXPECT_FALSE(modifier.isEditActive());
	const voxel::RawVolume *volume = _sceneMgr->sceneGraphNodeByUUID(uuid)->volume();
	EXPECT_EQ(1, volume->voxel(0, 0, 0).getColor());
	EXPECT_TRUE(voxel::isAir(volume->voxel(2, 0, 0).getMaterial()));
	ASSERT_TRUE(_sceneMgr->undo());
	volume = _sceneMgr->sceneGraphNodeByUUID(uuid)->volume();
	EXPECT_TRUE(voxel::isAir(volume->voxel(0, 0, 0).getMaterial()));
}

TEST_F(BrushEditSessionTest, testLineSettingsOnlyInvalidateWhenValuesChange) {
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Line);
	LineBrush &brush = modifier.lineBrush();
	ASSERT_TRUE(modifier.beginEdit());
	brush.markClean();
	const uint64_t revision = brush.settingsRevision();
	brush.setContinuous(brush.continuous());
	brush.setThickness(brush.thickness());
	brush.setSag(brush.sag());
	brush.setBezier(brush.bezier());
	EXPECT_EQ(revision, brush.settingsRevision());
	EXPECT_FALSE(brush.dirty());
	modifier.flushPendingBrushChanges();
	EXPECT_TRUE(modifier.isEditActive());
	brush.setContinuous(!brush.continuous());
	EXPECT_TRUE(brush.dirty());
	modifier.flushPendingBrushChanges();
	EXPECT_FALSE(modifier.isEditActive());
}

TEST_F(BrushEditSessionTest, testSceneManagerResizeFinishesEdit) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(m.beginEdit());
	ASSERT_TRUE(m.updateEdit());
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	_sceneMgr->nodeResize(node->uuid(), node->region().getDimensionsInVoxels() + glm::ivec3(1));
	EXPECT_FALSE(m.isEditActive());
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(0, 0, 0).getMaterial()));
}

TEST_F(BrushEditSessionTest, testDeleteTargetCommitsStrokeBeforeRemoval) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setStrokeMode();
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(m.beginEdit());
	ASSERT_TRUE(m.updateEdit());
	const core::UUID uuid = _sceneMgr->activeNodeUUID();
	ASSERT_TRUE(_sceneMgr->nodeRemove(uuid, false));
	EXPECT_FALSE(m.isEditActive());
	ASSERT_TRUE(_sceneMgr->undo());
	ASSERT_NE(nullptr, _sceneMgr->sceneGraphNodeByUUID(uuid));
	EXPECT_FALSE(voxel::isAir(_sceneMgr->volume(uuid)->voxel(0, 0, 0).getMaterial()));
	ASSERT_TRUE(_sceneMgr->undo());
	EXPECT_TRUE(voxel::isAir(_sceneMgr->volume(uuid)->voxel(0, 0, 0).getMaterial()));
}

TEST_F(BrushEditSessionTest, testStrokeAfterUnfinishedBoxReleasesNormally) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setBoxMode();
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	m.setCursorPosition(glm::ivec3(2, 0, 2), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(m.needsAdditionalAction());
	command::executeCommands("-actionexecute 1 0.05");
	m.shapeBrush().setStrokeMode();
	command::executeCommands("+actionexecute 1 0.1");
	ASSERT_TRUE(m.isEditActive());
	command::executeCommands("-actionexecute 1 0.15");
	EXPECT_FALSE(m.isEditActive());
}

TEST_F(BrushEditSessionTest, testReleaseDoesNotRepeatStationaryPaintDab) {
	Modifier &m = _sceneMgr->modifier();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->activeNode());
	ASSERT_NE(nullptr, node);
	node->palette().setColor(0, color::RGBA(40, 40, 40));
	node->palette().setColor(1, color::RGBA(80, 80, 80));
	node->palette().setColor(2, color::RGBA(160, 160, 160));
	node->palette().setSize(3);
	node->volume()->setVoxel(0, 0, 0, voxel::Voxel(voxel::VoxelType::Generic, 0));
	m.setBrushType(BrushType::Paint);
	m.paintBrush().setPaintMode(PaintBrush::PaintMode::Brighten);
	m.paintBrush().setStrength(1.0f);
	m.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	const voxel::Voxel firstDab = node->volume()->voxel(0, 0, 0);
	EXPECT_NE(0, firstDab.getColor());
	command::executeCommands("-actionexecute 1 0.05");
	EXPECT_EQ(firstDab, node->volume()->voxel(0, 0, 0));
}

TEST_F(BrushEditSessionTest, testBoxHandleWaitsForBrushPressToRelease) {
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setBrushType(BrushType::Shape);
	ShapeBrush &brush = modifier.shapeBrush();
	brush.setBoxMode();
	brush.setRadius(0);
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	core::ScopedBindingContext bindingContext(core::BindingContext::Context2);
	command::executeCommands("+actionexecute 1 0.0");
	ASSERT_TRUE(modifier.isEditActive());
	ASSERT_TRUE(modifier.isBrushActionActive());
	modifier.setCursorPosition(glm::ivec3(2, 0, 2), voxel::FaceNames::PositiveY);
	core::DynamicArray<BrushGizmoState> handles;
	modifier.brushHandles(handles);
	ASSERT_EQ(1u, handles.size());
	BrushHandleEvent event;
	event.operationId = handles[0].operationId;
	event.handleId = handles[0].id;
	event.viewportId = 1;
	event.phase = BrushHandlePhase::Begin;
	EXPECT_FALSE(modifier.editBrushHandle(event));
	EXPECT_FALSE(modifier.isBrushGizmoActive());
	command::executeCommands("-actionexecute 1 0.1");
	EXPECT_FALSE(modifier.isBrushActionActive());
	ASSERT_TRUE(modifier.isEditActive());
	EXPECT_TRUE(modifier.editBrushHandle(event));
	EXPECT_TRUE(modifier.isBrushGizmoActive());
	event.phase = BrushHandlePhase::End;
	EXPECT_TRUE(modifier.editBrushHandle(event));
	modifier.abort();
}

TEST_F(BrushEditSessionTest, testBoxModeUsesEditSession) {
	Modifier &m = _sceneMgr->modifier();
	m.setBrushType(BrushType::Shape);
	m.shapeBrush().setBoxMode();
	EXPECT_TRUE(m.usesEditSession());
	m.setBrushType(BrushType::Paint);
	m.paintBrush().setBoxMode();
	EXPECT_TRUE(m.usesEditSession());
	m.paintBrush().setFloodFill();
	EXPECT_TRUE(m.usesEditSession());
}

namespace {
class CountingBrush : public AABBBrush {
	uint8_t _samples = 0;

protected:
	void generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &input,
				  const voxel::Region &) override {
		wrapper.setVoxel(input.cursorPosition.x, input.cursorPosition.y, input.cursorPosition.z,
						 voxel::createVoxel(voxel::VoxelType::Generic, ++_samples));
	}

public:
	CountingBrush() : AABBBrush(BrushType::None) {
		setStrokeMode();
	}
	uint8_t samples() const {
		return _samples;
	}
};
} // namespace

TEST_F(BrushEditSessionTest, testRegisteredStatefulToolGetsIndependentTargetAndPreviewRuntime) {
	int firstId, secondId;
	lockTwoModels(firstId, secondId);
	Modifier &modifier = _sceneMgr->modifier();
	core::SharedPtr<CountingBrush> tool = core::make_shared<CountingBrush>();
	ASSERT_TRUE(modifier.registry().add("counting", tool));
	ASSERT_TRUE(modifier.setTool("counting"));
	modifier.setCursorPosition(glm::ivec3(0), voxel::FaceNames::PositiveY);
	ASSERT_TRUE(modifier.beginEdit());
	ASSERT_TRUE(modifier.updateEdit());
	EXPECT_EQ(1, volumeOf(firstId)->voxel(0, 0, 0).getColor());
	EXPECT_EQ(1, volumeOf(secondId)->voxel(0, 0, 0).getColor());
	modifier.setCursorPosition(glm::ivec3(1, 0, 0), voxel::FaceNames::PositiveY);
	scenegraph::SceneGraphNode &node = _sceneMgr->sceneGraph().node(firstId);
	const size_t states = _sceneMgr->mementoHandler().stateSize();
	for (int i = 0; i < 3; ++i) {
		modifier.previewManager().updateBrushVolumePreview(modifier, node.palette(), node.volume(),
														   _sceneMgr->sceneGraph());
		ASSERT_NE(nullptr, modifier.previewManager().previewVolume());
		EXPECT_EQ(2, modifier.previewManager().previewVolume()->voxel(1, 0, 0).getColor());
	}
	EXPECT_EQ(states, _sceneMgr->mementoHandler().stateSize());
	EXPECT_EQ(0, tool->samples());
	ASSERT_TRUE(modifier.updateEdit());
	modifier.finishEdit(BrushEditFinishReason::Release);
	EXPECT_EQ(2, volumeOf(firstId)->voxel(1, 0, 0).getColor());
	EXPECT_EQ(2, volumeOf(secondId)->voxel(1, 0, 0).getColor());
	ASSERT_TRUE(_sceneMgr->undo());
	EXPECT_TRUE(voxel::isAir(volumeOf(firstId)->voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volumeOf(secondId)->voxel(1, 0, 0).getMaterial()));
}

} // namespace voxedit
