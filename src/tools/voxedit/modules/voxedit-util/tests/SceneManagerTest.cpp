/**
 * @file
 */

#include "../SceneManager.h"
#include "../Config.h"
#include "video/tests/AbstractGLTest.h"
#include "voxel/RawVolume.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

class SceneManagerProtected : public voxedit::SceneManager {
protected:
	template<typename Volume>
	inline int countVoxels(const Volume& volume, const voxel::Voxel &voxel) {
		int cnt = 0;
		voxelutil::visitVolume(volume, [&](int, int, int, const voxel::Voxel &v) {
			if (v == voxel) {
				++cnt;
			}
		}, voxelutil::VisitAll());
		return cnt;
	}
};

class SceneManagerTest : public video::AbstractGLTest, public SceneManagerProtected {
protected:
	voxelrender::RenderContext _renderContext;

	void SetUp() override {
		video::AbstractGLTest::SetUp();
		if (IsSkipped()) {
			return;
		}
		video::ShaderVarState state;
		setShaderVars(state);
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::VoxEditShowaabb, "");
		core::Var::get(cfg::VoxEditGrayInactive, "");
		core::Var::get(cfg::VoxEditHideInactive, "");
		core::Var::get(cfg::VoxEditLastPalette, "");
		construct();
		_renderContext.init(video::getWindowSize());
		ASSERT_TRUE(init());

		const voxel::Region region{0, 1};
		ASSERT_TRUE(newScene(true, "newscene", region));

		modifier().setCursorVoxel(voxel::createVoxel(1));
		modifier().setModifierType(ModifierType::Place);
		modifier().setPlaneMode(false);
		modifier().setSingleMode(true);
		EXPECT_FALSE(mementoHandler().canUndo());
		EXPECT_FALSE(mementoHandler().canRedo());
	}

	void testSetVoxel(const glm::ivec3 &pos, int paletteColorIndex = 1) {
		modifier().setCursorPosition(pos, voxel::FaceNames::NegativeX);
		modifier().setCursorVoxel(voxel::createVoxel(paletteColorIndex));
		modifier().aabbStart();
		const int nodeId = sceneGraph().activeNode();
		voxel::RawVolume *v = volume(nodeId);
		if (!voxel::isAir(v->voxel(pos).getMaterial())) {
			modifier().setModifierType(ModifierType::Paint);
		}
		modifier().aabbAction(v, [&](const voxel::Region &region, ModifierType, bool) { modified(nodeId, region); });
		modifier().setModifierType(ModifierType::Place);
	}

	void testSelect(const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		modifier().aabbAbort();
		modifier().setSingleMode(false);
		modifier().setPlaneMode(false);
		modifier().setModifierType(ModifierType::Select);
		modifier().setCursorPosition(mins, voxel::FaceNames::NegativeX);
		EXPECT_TRUE(modifier().aabbStart());
		modifier().setCursorPosition(maxs, voxel::FaceNames::NegativeX);
		modifier().aabbStep();
		EXPECT_TRUE(modifier().aabbAction(nullptr, [&](const voxel::Region &, ModifierType, bool) {}));
		modifier().setModifierType(ModifierType::Place);
	}

	voxel::RawVolume *testVolume() {
		const int nodeId = sceneGraph().activeNode();
		voxel::RawVolume *v = volume(nodeId);
		return v;
	}

	glm::ivec3 testMins() {
		return testVolume()->region().getLowerCorner();
	}

	glm::ivec3 testMaxs() {
		return testVolume()->region().getUpperCorner();
	}

	void TearDown() override {
		if (!IsSkipped()) {
			_renderContext.shutdown();
			shutdown();
		}
		video::AbstractGLTest::TearDown();
	}
};

TEST_F(SceneManagerTest, testNewScene) {
	EXPECT_TRUE(newScene(true, "newscene", voxel::Region{0, 1}));
}

TEST_F(SceneManagerTest, testUndoRedoModification) {
	EXPECT_FALSE(dirty());
	testSetVoxel(testMins());
	EXPECT_TRUE(dirty());

	for (int i = 0; i < 3; ++i) {
		EXPECT_TRUE(mementoHandler().canUndo());
		EXPECT_TRUE(voxel::isBlocked(testVolume()->voxel(0, 0, 0).getMaterial()));
		EXPECT_TRUE(undo());
		// EXPECT_FALSE(dirty()); see todo at undo() and activate me
		EXPECT_FALSE(mementoHandler().canUndo());
		EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()));

		EXPECT_TRUE(redo());
		EXPECT_TRUE(dirty());
		EXPECT_TRUE(mementoHandler().canUndo());
		EXPECT_FALSE(mementoHandler().canRedo());
		EXPECT_TRUE(voxel::isBlocked(testVolume()->voxel(0, 0, 0).getMaterial()));
	}
}

TEST_F(SceneManagerTest, testNodeAddUndoRedo) {
	EXPECT_NE(-1, addModelChild("second node", 1, 1, 1));
	EXPECT_NE(-1, addModelChild("third node", 1, 1, 1));

	EXPECT_TRUE(mementoHandler().canUndo());
	EXPECT_FALSE(mementoHandler().canRedo());
	EXPECT_EQ(3u, sceneGraph().size());

	for (int i = 0; i < 3; ++i) {
		{
			EXPECT_TRUE(undo());
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(mementoHandler().canRedo());
			EXPECT_EQ(2u, sceneGraph().size());
		}
		{
			EXPECT_TRUE(undo());
			EXPECT_FALSE(mementoHandler().canUndo());
			EXPECT_TRUE(mementoHandler().canRedo());
			EXPECT_EQ(1u, sceneGraph().size());
		}
		{
			EXPECT_TRUE(redo());
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(mementoHandler().canRedo());
			EXPECT_EQ(2u, sceneGraph().size());
		}
		{
			EXPECT_TRUE(redo());
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_FALSE(mementoHandler().canRedo());
			EXPECT_EQ(3u, sceneGraph().size());
		}
	}
}

TEST_F(SceneManagerTest, testUndoRedoModificationMultipleNodes) {
	EXPECT_EQ(1u, mementoHandler().stateSize());
	// modification
	testSetVoxel(testMins(), 1);
	EXPECT_EQ(2u, mementoHandler().stateSize());

	// new node
	EXPECT_NE(-1, addModelChild("second node", 1, 1, 1));
	EXPECT_EQ(3u, mementoHandler().stateSize());

	// modification of the new node
	testSetVoxel(testMins(), 2);
	EXPECT_EQ(4u, mementoHandler().stateSize());

	// modification of the new node
	testSetVoxel(testMins(), 3);
	EXPECT_EQ(5u, mementoHandler().stateSize());

	// last state is the active state
	EXPECT_EQ(4u, mementoHandler().statePosition());

	for (int i = 0; i < 3; ++i) {
		const int nodeId = sceneGraph().activeNode();
		EXPECT_EQ(3, testVolume()->voxel(0, 0, 0).getColor());
		{
			// undo modification in second volume
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(undo());
			EXPECT_EQ(2, testVolume()->voxel(0, 0, 0).getColor());
			EXPECT_EQ(nodeId, sceneGraph().activeNode());
		}
		{
			// undo modification in second volume
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(undo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
			EXPECT_EQ(nodeId, sceneGraph().activeNode());
		}
		{
			// undo adding a new node
			EXPECT_EQ(2u, sceneGraph().size());
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(undo());
			EXPECT_EQ(1u, sceneGraph().size());
			EXPECT_NE(nodeId, sceneGraph().activeNode());
		}
		{
			// undo modification in first volume
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
			EXPECT_TRUE(undo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
		}
		{
			// redo modification in first volume
			EXPECT_FALSE(mementoHandler().canUndo());
			EXPECT_TRUE(mementoHandler().canRedo());
			EXPECT_TRUE(redo());
			EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
		}
		{
			// redo add new node
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(mementoHandler().canRedo());
			EXPECT_TRUE(redo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
		}
		{
			// redo modification in second volume
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(mementoHandler().canRedo());
			EXPECT_TRUE(redo());
			EXPECT_EQ(2, testVolume()->voxel(0, 0, 0).getColor());
		}
		{
			// redo modification in second volume
			EXPECT_TRUE(mementoHandler().canUndo());
			EXPECT_TRUE(mementoHandler().canRedo());
			EXPECT_TRUE(redo());
			EXPECT_EQ(3, testVolume()->voxel(0, 0, 0).getColor());
		}
		EXPECT_FALSE(mementoHandler().canRedo());
	}
}

TEST_F(SceneManagerTest, testRenameUndoRedo) {
	EXPECT_EQ(1u, mementoHandler().stateSize());
	EXPECT_TRUE(nodeRename(sceneGraph().activeNode(), "newname"));
	EXPECT_EQ(2u, mementoHandler().stateSize());

	for (int i = 0; i < 3; ++i) {
		EXPECT_TRUE(mementoHandler().canUndo());
		EXPECT_FALSE(mementoHandler().canRedo());
		EXPECT_TRUE(undo());
		EXPECT_FALSE(mementoHandler().canUndo());
		EXPECT_TRUE(mementoHandler().canRedo());
		EXPECT_TRUE(redo());
	}
	const int nodeId = sceneGraph().activeNode();
	EXPECT_EQ("newname", sceneGraph().node(nodeId).name());
}

TEST_F(SceneManagerTest, testCopyPaste) {
	testSetVoxel(testMins(), 1);
	testSelect(testMins(), testMaxs());
	EXPECT_TRUE(modifier().selection().isValid());
	EXPECT_TRUE(copy());

	EXPECT_NE(-1, addModelChild("paste target", 1, 1, 1));
	EXPECT_TRUE(paste(testMins()));
	EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
}

TEST_F(SceneManagerTest, testMergeSimple) {
	int secondNodeId = addModelChild("second node", 10, 10, 10);
	int thirdNodeId = addModelChild("third node", 10, 10, 10);
	ASSERT_NE(-1, secondNodeId);
	ASSERT_NE(-1, thirdNodeId);

	// set voxel into second node
	EXPECT_TRUE(nodeActivate(secondNodeId));
	testSetVoxel(glm::ivec3(1, 1, 1));
	EXPECT_EQ(1, countVoxels(*volume(secondNodeId), modifier().cursorVoxel()));

	// set voxel into third node
	EXPECT_TRUE(nodeActivate(thirdNodeId));
	testSetVoxel(glm::ivec3(2, 2, 2));
	EXPECT_EQ(1, countVoxels(*volume(thirdNodeId), modifier().cursorVoxel()));

	// merge and validate
	int newNodeId = mergeNodes(secondNodeId, thirdNodeId);
	const voxel::RawVolume *v = volume(newNodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(2, countVoxels(*v, modifier().cursorVoxel()));
	EXPECT_FALSE(voxel::isAir(v->voxel(glm::ivec3(1, 1, 1)).getMaterial()));
	EXPECT_FALSE(voxel::isAir(v->voxel(glm::ivec3(2, 2, 2)).getMaterial()));

	// merged nodes are gone
	EXPECT_EQ(nullptr, sceneGraphNode(secondNodeId));
	EXPECT_EQ(nullptr, sceneGraphNode(thirdNodeId));
}

TEST_F(SceneManagerTest, DISABLED_testMergeTransform) {
	// TODO:
}

} // namespace voxedit
