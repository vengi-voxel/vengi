/**
 * @file
 */

#include "../SceneManager.h"
#include "../Config.h"
#include "app/tests/AbstractTest.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

class SceneManagerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	SceneManager _sceneMgr{core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>()};

	template<typename Volume>
	inline int countVoxels(const Volume &volume, const voxel::Voxel &voxel) {
		int cnt = 0;
		voxelutil::visitVolume(
			volume,
			[&](int, int, int, const voxel::Voxel &v) {
				if (v == voxel) {
					++cnt;
				}
			},
			voxelutil::VisitAll());
		return cnt;
	}

	void TearDown() override {
		_sceneMgr.shutdown();
		Super::TearDown();
	}

	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::VoxEditShowgrid, "true");
		core::Var::get(cfg::VoxEditShowlockedaxis, "true");
		core::Var::get(cfg::VoxEditRendershadow, "true");
		core::Var::get(cfg::VoxEditGridsize, "1");
		core::Var::get(cfg::VoxelMeshMode, "0");
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::VoxEditShowaabb, "");
		core::Var::get(cfg::VoxEditGrayInactive, "");
		core::Var::get(cfg::VoxEditHideInactive, "");
		core::Var::get(cfg::VoxEditLastPalette, "");
		core::Var::get(cfg::VoxEditModificationDismissMillis, "0");
		_sceneMgr.construct();
		ASSERT_TRUE(_sceneMgr.init());

		const voxel::Region region{0, 1};
		ASSERT_TRUE(_sceneMgr.newScene(true, "newscene", region));

		Modifier &modifier = _sceneMgr.modifier();
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setModifierType(ModifierType::Place);
		modifier.shapeBrush().setSingleMode(true);
		MementoHandler &mementoHandler = _sceneMgr.mementoHandler();
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
	}

	void testSetVoxel(const glm::ivec3 &pos, int paletteColorIndex = 1) {
		Modifier &modifier = _sceneMgr.modifier();
		modifier.setCursorPosition(pos, voxel::FaceNames::NegativeX);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, paletteColorIndex));
		modifier.start();
		const int nodeId = _sceneMgr.sceneGraph().activeNode();
		voxel::RawVolume *v = _sceneMgr.volume(nodeId);
		if (!voxel::isAir(v->voxel(pos).getMaterial())) {
			modifier.setModifierType(ModifierType::Paint);
		}
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(v, false);
		modifier.execute(sceneGraph, node,
						 [&](const voxel::Region &region, ModifierType, bool) { _sceneMgr.modified(nodeId, region); });
		modifier.setModifierType(ModifierType::Place);
	}

	void testSelect(const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		Modifier &modifier = _sceneMgr.modifier();
		modifier.stop();
		modifier.shapeBrush().setSingleMode(false);
		modifier.setModifierType(ModifierType::Select);
		modifier.setCursorPosition(mins, voxel::FaceNames::NegativeX);
		EXPECT_TRUE(modifier.start());
		modifier.setCursorPosition(maxs, voxel::FaceNames::NegativeX);
		modifier.executeAdditionalAction();
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		EXPECT_TRUE(modifier.execute(sceneGraph, node, [&](const voxel::Region &, ModifierType, bool) {}));
		modifier.setModifierType(ModifierType::Place);
	}

	voxel::RawVolume *testVolume() {
		const int nodeId = _sceneMgr.sceneGraph().activeNode();
		voxel::RawVolume *v = _sceneMgr.volume(nodeId);
		return v;
	}

	glm::ivec3 testMins() {
		return testVolume()->region().getLowerCorner();
	}

	glm::ivec3 testMaxs() {
		return testVolume()->region().getUpperCorner();
	}
};

TEST_F(SceneManagerTest, testNewScene) {
	EXPECT_TRUE(_sceneMgr.newScene(true, "newscene", voxel::Region{0, 1}));
}

TEST_F(SceneManagerTest, testUndoRedoModification) {
	EXPECT_FALSE(_sceneMgr.dirty());
	testSetVoxel(testMins());
	EXPECT_TRUE(_sceneMgr.dirty());

	MementoHandler &mementoHandler = _sceneMgr.mementoHandler();
	for (int i = 0; i < 3; ++i) {
		EXPECT_TRUE(mementoHandler.canUndo());
		EXPECT_TRUE(voxel::isBlocked(testVolume()->voxel(0, 0, 0).getMaterial()));
		EXPECT_TRUE(_sceneMgr.undo());
		// EXPECT_FALSE(dirty()); see todo at undo() and activate me
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()));

		EXPECT_TRUE(_sceneMgr.redo());
		EXPECT_TRUE(_sceneMgr.dirty());
		EXPECT_TRUE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
		EXPECT_TRUE(voxel::isBlocked(testVolume()->voxel(0, 0, 0).getMaterial()));
	}
}

TEST_F(SceneManagerTest, testNodeAddUndoRedo) {
	EXPECT_NE(-1, _sceneMgr.addModelChild("second node", 1, 1, 1));
	EXPECT_NE(-1, _sceneMgr.addModelChild("third node", 1, 1, 1));

	MementoHandler &mementoHandler = _sceneMgr.mementoHandler();
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_EQ(3u, _sceneMgr.sceneGraph().size());

	for (int i = 0; i < 3; ++i) {
		{
			EXPECT_TRUE(_sceneMgr.undo());
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(2u, _sceneMgr.sceneGraph().size());
		}
		{
			EXPECT_TRUE(_sceneMgr.undo());
			EXPECT_FALSE(mementoHandler.canUndo());
			EXPECT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(1u, _sceneMgr.sceneGraph().size());
		}
		{
			EXPECT_TRUE(_sceneMgr.redo());
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(2u, _sceneMgr.sceneGraph().size());
		}
		{
			EXPECT_TRUE(_sceneMgr.redo());
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_FALSE(mementoHandler.canRedo());
			EXPECT_EQ(3u, _sceneMgr.sceneGraph().size());
		}
	}
}

TEST_F(SceneManagerTest, testUndoRedoModificationMultipleNodes) {
	MementoHandler &mementoHandler = _sceneMgr.mementoHandler();
	EXPECT_EQ(1u, mementoHandler.stateSize());
	// modification
	testSetVoxel(testMins(), 1);
	EXPECT_EQ(2u, mementoHandler.stateSize());

	// new node
	EXPECT_NE(-1, _sceneMgr.addModelChild("second node", 1, 1, 1));
	EXPECT_EQ(3u, mementoHandler.stateSize());

	// modification of the new node
	testSetVoxel(testMins(), 2);
	EXPECT_EQ(4u, mementoHandler.stateSize());

	// modification of the new node
	testSetVoxel(testMins(), 3);
	EXPECT_EQ(5u, mementoHandler.stateSize());

	// last state is the active state
	EXPECT_EQ(4u, mementoHandler.statePosition());

	for (int i = 0; i < 3; ++i) {
		const int nodeId = _sceneMgr.sceneGraph().activeNode();
		EXPECT_EQ(3, testVolume()->voxel(0, 0, 0).getColor());
		{
			// undo modification in second volume
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(_sceneMgr.undo());
			EXPECT_EQ(2, testVolume()->voxel(0, 0, 0).getColor());
			EXPECT_EQ(nodeId, _sceneMgr.sceneGraph().activeNode());
		}
		{
			// undo modification in second volume
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(_sceneMgr.undo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
			EXPECT_EQ(nodeId, _sceneMgr.sceneGraph().activeNode());
		}
		{
			// undo adding a new node
			EXPECT_EQ(2u, _sceneMgr.sceneGraph().size());
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(_sceneMgr.undo());
			EXPECT_EQ(1u, _sceneMgr.sceneGraph().size());
			EXPECT_NE(nodeId, _sceneMgr.sceneGraph().activeNode());
		}
		{
			// undo modification in first volume
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
			EXPECT_TRUE(_sceneMgr.undo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
		}
		{
			// redo modification in first volume
			EXPECT_FALSE(mementoHandler.canUndo());
			EXPECT_TRUE(mementoHandler.canRedo());
			EXPECT_TRUE(_sceneMgr.redo());
			EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
		}
		{
			// redo add new node
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(mementoHandler.canRedo());
			EXPECT_TRUE(_sceneMgr.redo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
		}
		{
			// redo modification in second volume
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(mementoHandler.canRedo());
			EXPECT_TRUE(_sceneMgr.redo());
			EXPECT_EQ(2, testVolume()->voxel(0, 0, 0).getColor());
		}
		{
			// redo modification in second volume
			EXPECT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(mementoHandler.canRedo());
			EXPECT_TRUE(_sceneMgr.redo());
			EXPECT_EQ(3, testVolume()->voxel(0, 0, 0).getColor());
		}
		EXPECT_FALSE(mementoHandler.canRedo());
	}
}

TEST_F(SceneManagerTest, testRenameUndoRedo) {
	MementoHandler &mementoHandler = _sceneMgr.mementoHandler();
	EXPECT_EQ(1u, mementoHandler.stateSize());
	EXPECT_TRUE(_sceneMgr.nodeRename(_sceneMgr.sceneGraph().activeNode(), "newname"));
	EXPECT_EQ(2u, mementoHandler.stateSize());

	for (int i = 0; i < 3; ++i) {
		EXPECT_TRUE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
		EXPECT_TRUE(_sceneMgr.undo());
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_TRUE(mementoHandler.canRedo());
		EXPECT_TRUE(_sceneMgr.redo());
	}
	const int nodeId = _sceneMgr.sceneGraph().activeNode();
	EXPECT_EQ("newname", _sceneMgr.sceneGraph().node(nodeId).name());
}

TEST_F(SceneManagerTest, testCopyPaste) {
	Modifier &modifier = _sceneMgr.modifier();
	testSetVoxel(testMins(), 1);
	testSelect(testMins(), testMaxs());
	EXPECT_FALSE(modifier.selections().empty());
	EXPECT_TRUE(_sceneMgr.copy());

	EXPECT_NE(-1, _sceneMgr.addModelChild("paste target", 1, 1, 1));
	EXPECT_TRUE(_sceneMgr.paste(testMins()));
	EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
}

TEST_F(SceneManagerTest, testMergeSimple) {
	Modifier &modifier = _sceneMgr.modifier();
	int secondNodeId = _sceneMgr.addModelChild("second node", 10, 10, 10);
	int thirdNodeId = _sceneMgr.addModelChild("third node", 10, 10, 10);
	ASSERT_NE(-1, secondNodeId);
	ASSERT_NE(-1, thirdNodeId);

	// set voxel into second node
	EXPECT_TRUE(_sceneMgr.nodeActivate(secondNodeId));
	testSetVoxel(glm::ivec3(1, 1, 1));
	EXPECT_EQ(1, countVoxels(*_sceneMgr.volume(secondNodeId), modifier.cursorVoxel()));

	// set voxel into third node
	EXPECT_TRUE(_sceneMgr.nodeActivate(thirdNodeId));
	testSetVoxel(glm::ivec3(2, 2, 2));
	EXPECT_EQ(1, countVoxels(*_sceneMgr.volume(thirdNodeId), modifier.cursorVoxel()));

	// merge and validate
	int newNodeId = _sceneMgr.mergeNodes(secondNodeId, thirdNodeId);
	const voxel::RawVolume *v = _sceneMgr.volume(newNodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(2, countVoxels(*v, modifier.cursorVoxel()));
	EXPECT_FALSE(voxel::isAir(v->voxel(glm::ivec3(1, 1, 1)).getMaterial()));
	EXPECT_FALSE(voxel::isAir(v->voxel(glm::ivec3(2, 2, 2)).getMaterial()));

	// merged nodes are gone
	EXPECT_EQ(nullptr, _sceneMgr.sceneGraphNode(secondNodeId));
	EXPECT_EQ(nullptr, _sceneMgr.sceneGraphNode(thirdNodeId));
}

TEST_F(SceneManagerTest, DISABLED_testMergeTransform) {
	// TODO:
}

} // namespace voxedit
