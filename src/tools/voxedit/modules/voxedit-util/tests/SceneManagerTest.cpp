/**
 * @file
 */

#include "../SceneManager.h"
#include "../Config.h"
#include "app/tests/AbstractTest.h"
#include "core/TimeProvider.h"
#include "math/tests/TestMathHelper.h"
#include "palette/Palette.h"
#include "palette/tests/TestHelper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/tests/TestHelper.h"
#include "util/VarUtil.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

class SceneManagerEx : public SceneManager {
public:
	SceneManagerEx(const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem,
				   const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer)
		: SceneManager(timeProvider, filesystem, sceneRenderer, modifierRenderer) {
	}
	bool loadForTest(scenegraph::SceneGraph &&sceneGraph) {
		return loadSceneGraph(core::move(sceneGraph));
	}

	void setLastFilename(const core::String &name, const io::FormatDescription *desc = nullptr) {
		_lastFilename.set(name, desc);
	}

	void clearLastFilename() {
		_lastFilename.clear();
	}
};

class SceneManagerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	SceneManagerPtr _sceneMgr;
	SceneManagerEx *sceneMgr() {
		return ((SceneManagerEx *)_sceneMgr.get());
	}

	template<typename Volume>
	inline int countVoxels(const Volume &volume, const voxel::Voxel &voxel) {
		return voxelutil::visitVolume(
			volume,
			[&](int, int, int, const voxel::Voxel &v) {
			},
			voxelutil::VisitColor(voxel.getColor()));
	}

	void TearDown() override {
		_sceneMgr->shutdown();
		_sceneMgr.release();
		Super::TearDown();
	}

	void SetUp() override {
		Super::SetUp();
		const auto timeProvider = core::make_shared<core::TimeProvider>();
		const auto sceneRenderer = core::make_shared<ISceneRenderer>();
		const auto modifierRenderer = core::make_shared<IModifierRenderer>();
		_sceneMgr =
			core::make_shared<SceneManagerEx>(timeProvider, _testApp->filesystem(), sceneRenderer, modifierRenderer);
		core::Var::get(cfg::VoxEditShowgrid, "true");
		core::Var::get(cfg::VoxEditShowlockedaxis, "true");
		core::Var::get(cfg::VoxEditRendershadow, "true");
		core::Var::get(cfg::VoxEditGridsize, "1");
		core::Var::get(cfg::VoxEditPlaneSize, "0");
		core::Var::get(cfg::VoxEditShowPlane, "false");
		core::Var::get(cfg::VoxEditMaxSuggestedVolumeSize, "128");
		core::Var::get(cfg::VoxEditMaxSuggestedVolumeSizePreview, "32");
		core::Var::get(cfg::VoxelMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Binary));
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::VoxEditShowaabb, "");
		core::Var::get(cfg::VoxEditShowBones, "");
		core::Var::get(cfg::VoxEditGrayInactive, "");
		core::Var::get(cfg::VoxEditHideInactive, "");
		core::Var::get(cfg::VoxEditLastPalette, "");
		core::Var::get(cfg::VoxEditModificationDismissMillis, "0");
		_sceneMgr->construct();
		ASSERT_TRUE(_sceneMgr->init());

		const voxel::Region region{0, 1};
		ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));

		Modifier &modifier = _sceneMgr->modifier();
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setBrushType(BrushType::Shape);
		modifier.setModifierType(ModifierType::Place);
		memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
	}

	bool testSetVoxel(const glm::ivec3 &pos, int paletteColorIndex = 1) {
		Modifier &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::Shape);
		modifier.shapeBrush().setSingleMode();
		modifier.setModifierType(ModifierType::Override);
		modifier.setCursorPosition(pos, voxel::FaceNames::NegativeX);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, paletteColorIndex));
		if (!modifier.start()) {
			return false;
		}
		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		voxel::RawVolume *v = _sceneMgr->volume(nodeId);
		if (v == nullptr) {
			return false;
		}
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(v, false);
		int executed = 0;
		auto callback = [&](const voxel::Region &region, ModifierType, bool) {
			executed++;
			_sceneMgr->modified(nodeId, region);
		};
		if (!modifier.execute(sceneGraph, node, callback)) {
			return false;
		}
		return executed == 1;
	}

	void testSelect(const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		Modifier &modifier = _sceneMgr->modifier();
		modifier.stop();
		modifier.setBrushType(BrushType::Select);
		modifier.setCursorPosition(mins, voxel::FaceNames::NegativeX);
		EXPECT_TRUE(modifier.start());
		modifier.setCursorPosition(maxs, voxel::FaceNames::NegativeX);
		modifier.executeAdditionalAction();
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume({mins, maxs}), true);
		EXPECT_TRUE(modifier.execute(sceneGraph, node, [&](const voxel::Region &, ModifierType, bool) {}));
		modifier.setBrushType(BrushType::Shape);
	}

	voxel::RawVolume *testVolume() {
		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		voxel::RawVolume *v = _sceneMgr->volume(nodeId);
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
	EXPECT_TRUE(_sceneMgr->newScene(true, "newscene", voxel::Region{0, 1}));
}

TEST_F(SceneManagerTest, testUndoRedoModification) {
	EXPECT_FALSE(_sceneMgr->dirty());
	ASSERT_TRUE(testSetVoxel(testMins()));
	EXPECT_TRUE(_sceneMgr->dirty());

	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	for (int i = 0; i < 3; ++i) {
		ASSERT_TRUE(mementoHandler.canUndo());
		EXPECT_TRUE(voxel::isBlocked(testVolume()->voxel(0, 0, 0).getMaterial()));
		EXPECT_TRUE(_sceneMgr->undo());
		// EXPECT_FALSE(dirty()); see todo at undo() and activate me
		ASSERT_FALSE(mementoHandler.canUndo());
		EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()));

		ASSERT_TRUE(mementoHandler.canRedo());
		EXPECT_TRUE(_sceneMgr->redo());
		EXPECT_TRUE(_sceneMgr->dirty());
		ASSERT_TRUE(mementoHandler.canUndo());
		ASSERT_FALSE(mementoHandler.canRedo());
		EXPECT_TRUE(voxel::isBlocked(testVolume()->voxel(0, 0, 0).getMaterial()));
	}
}

TEST_F(SceneManagerTest, testNodeAddUndoRedo) {
	EXPECT_NE(InvalidNodeId, _sceneMgr->addModelChild("second node", 1, 1, 1));
	EXPECT_NE(InvalidNodeId, _sceneMgr->addModelChild("third node", 1, 1, 1));
	EXPECT_EQ(3u, _sceneMgr->mementoHandler().stateSize());

	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_EQ(3u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();

	for (int i = 0; i < 3; ++i) {
		SCOPED_TRACE(i);
		{
			EXPECT_EQ(2u, mementoHandler.statePosition());
			ASSERT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(_sceneMgr->undo());
			EXPECT_EQ(1u, mementoHandler.statePosition());
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(2u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
		}
		{
			EXPECT_TRUE(_sceneMgr->undo());
			ASSERT_FALSE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(1u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
		}
		{
			EXPECT_TRUE(_sceneMgr->redo());
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(2u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
		}
		{
			EXPECT_TRUE(_sceneMgr->redo());
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_FALSE(mementoHandler.canRedo());
			EXPECT_EQ(3u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
		}
	}
}

TEST_F(SceneManagerTest, testUndoRedoModificationMultipleNodes) {
	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	EXPECT_EQ(1u, mementoHandler.stateSize());
	// modification
	ASSERT_TRUE(testSetVoxel(testMins(), 1));
	EXPECT_EQ(2u, mementoHandler.stateSize());

	// new node
	EXPECT_NE(-1, _sceneMgr->addModelChild("second node", 1, 1, 1));
	EXPECT_EQ(3u, mementoHandler.stateSize());

	// modification of the new node
	ASSERT_TRUE(testSetVoxel(testMins(), 2));
	EXPECT_EQ(4u, mementoHandler.stateSize());

	// modification of the new node
	ASSERT_TRUE(testSetVoxel(testMins(), 3));
	EXPECT_EQ(5u, mementoHandler.stateSize());

	// last state is the active state
	EXPECT_EQ(4u, mementoHandler.statePosition());

	for (int i = 0; i < 3; ++i) {
		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		EXPECT_EQ(3, testVolume()->voxel(0, 0, 0).getColor());
		{
			// undo modification in second volume
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(_sceneMgr->undo());
			EXPECT_EQ(2, testVolume()->voxel(0, 0, 0).getColor());
			EXPECT_EQ(nodeId, _sceneMgr->sceneGraph().activeNode());
		}
		{
			// undo modification in second volume
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(_sceneMgr->undo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
			EXPECT_EQ(nodeId, _sceneMgr->sceneGraph().activeNode());
		}
		{
			// undo adding a new node
			EXPECT_EQ(2u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(_sceneMgr->undo());
			EXPECT_EQ(1u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
			EXPECT_NE(nodeId, _sceneMgr->sceneGraph().activeNode());
		}
		{
			// undo modification in first volume
			ASSERT_TRUE(mementoHandler.canUndo());
			EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
			ASSERT_TRUE(_sceneMgr->undo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
		}
		{
			// redo modification in first volume
			ASSERT_FALSE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			ASSERT_TRUE(_sceneMgr->redo());
			EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
		}
		{
			// redo add new node
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			ASSERT_TRUE(_sceneMgr->redo());
			EXPECT_TRUE(voxel::isAir(testVolume()->voxel(0, 0, 0).getMaterial()))
				<< "color is " << (int)testVolume()->voxel(0, 0, 0).getColor();
		}
		{
			// redo modification in second volume
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_TRUE(_sceneMgr->redo());
			EXPECT_EQ(2, testVolume()->voxel(0, 0, 0).getColor());
		}
		{
			// redo modification in second volume
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_TRUE(_sceneMgr->redo());
			EXPECT_EQ(3, testVolume()->voxel(0, 0, 0).getColor());
		}
		ASSERT_FALSE(mementoHandler.canRedo());
	}
}

TEST_F(SceneManagerTest, testRenameUndoRedo) {
	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	EXPECT_EQ(1u, mementoHandler.stateSize());
	EXPECT_TRUE(_sceneMgr->nodeRename(_sceneMgr->sceneGraph().activeNode(), "newname"));
	EXPECT_EQ(2u, mementoHandler.stateSize());

	for (int i = 0; i < 3; ++i) {
		EXPECT_TRUE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
		EXPECT_TRUE(_sceneMgr->undo());
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_TRUE(mementoHandler.canRedo());
		EXPECT_TRUE(_sceneMgr->redo());
	}
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	EXPECT_EQ("newname", _sceneMgr->sceneGraph().node(nodeId).name());
}

TEST_F(SceneManagerTest, testCopyPaste) {
	Modifier &modifier = _sceneMgr->modifier();
	testSetVoxel(testMins(), 1);
	testSelect(testMins(), testMaxs());
	EXPECT_TRUE(modifier.selectionMgr().hasSelection());
	EXPECT_TRUE(_sceneMgr->copy());

	EXPECT_NE(-1, _sceneMgr->addModelChild("paste target", 1, 1, 1));
	EXPECT_TRUE(_sceneMgr->paste(testMins()));
	EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
}

TEST_F(SceneManagerTest, testExceedsMaxSuggestedVolumeSize) {
	util::ScopedVarChange scoped(cfg::VoxEditMaxSuggestedVolumeSize, "16");
	const voxel::Region region{0, 32};
	ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));
	ASSERT_TRUE(_sceneMgr->exceedsMaxSuggestedVolumeSize());

	const voxel::Region regionSmall{0, 15};
	ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", regionSmall));
	ASSERT_FALSE(_sceneMgr->exceedsMaxSuggestedVolumeSize());
}

TEST_F(SceneManagerTest, testMergeSimple) {
	Modifier &modifier = _sceneMgr->modifier();
	int secondNodeId = _sceneMgr->addModelChild("second node", 10, 10, 10);
	int thirdNodeId = _sceneMgr->addModelChild("third node", 10, 10, 10);
	ASSERT_NE(-1, secondNodeId);
	ASSERT_NE(-1, thirdNodeId);

	// set voxel into second node
	EXPECT_TRUE(_sceneMgr->nodeActivate(secondNodeId));
	testSetVoxel(glm::ivec3(1, 1, 1), modifier.cursorVoxel().getColor());
	EXPECT_EQ(1, countVoxels(*_sceneMgr->volume(secondNodeId), modifier.cursorVoxel()));

	// set voxel into third node
	EXPECT_TRUE(_sceneMgr->nodeActivate(thirdNodeId));
	testSetVoxel(glm::ivec3(2, 2, 2), modifier.cursorVoxel().getColor());
	EXPECT_EQ(1, countVoxels(*_sceneMgr->volume(thirdNodeId), modifier.cursorVoxel()));

	// merge and validate
	int newNodeId = _sceneMgr->mergeNodes(secondNodeId, thirdNodeId);
	const voxel::RawVolume *v = _sceneMgr->volume(newNodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(2, voxelutil::visitVolume(*v, [&](int, int, int, const voxel::Voxel &voxel) {}, voxelutil::SkipEmpty()));
	EXPECT_FALSE(voxel::isAir(v->voxel(glm::ivec3(1, 1, 1)).getMaterial()));
	EXPECT_FALSE(voxel::isAir(v->voxel(glm::ivec3(2, 2, 2)).getMaterial()));

	// merged nodes are gone
	EXPECT_EQ(nullptr, _sceneMgr->sceneGraphNode(secondNodeId));
	EXPECT_EQ(nullptr, _sceneMgr->sceneGraphNode(thirdNodeId));
}

TEST_F(SceneManagerTest, testDuplicateNodeKeyFrame) {
	scenegraph::SceneGraphTransform transform;
	transform.setWorldTranslation(glm::vec3(100.0f, 0.0, 0.0f));

	EXPECT_TRUE(_sceneMgr->nodeAddKeyFrame(1, 1));
	EXPECT_TRUE(_sceneMgr->nodeAddKeyFrame(1, 10));
	EXPECT_TRUE(_sceneMgr->nodeAddKeyFrame(1, 20));

	scenegraph::SceneGraphNode &node = _sceneMgr->sceneGraph().node(1);
	node.keyFrame(2).setTransform(transform);
	_sceneMgr->sceneGraph().updateTransforms();

	EXPECT_TRUE(_sceneMgr->nodeAddKeyFrame(1, 15))
		<< "Expected to insert a new key frame at index 3 (sorting by frameIdx)";
	EXPECT_FLOAT_EQ(100.0f, node.keyFrame(3).transform().worldTranslation().x)
		<< "Expected to get the transform of key frame 2";

	EXPECT_TRUE(_sceneMgr->nodeAddKeyFrame(1, 30));
	EXPECT_FLOAT_EQ(0.0f, node.keyFrame(5).transform().worldTranslation().x);
}

TEST_F(SceneManagerTest, testRemoveUnusedColors) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId);
	ASSERT_NE(nullptr, node) << "Failed to get node for id " << nodeId;
	EXPECT_TRUE(testSetVoxel(testMins(), 1));
	const palette::Palette &palette = node->palette();
	EXPECT_EQ(palette::PaletteMaxColors, (int)palette.size());
	_sceneMgr->nodeRemoveUnusedColors(nodeId, true);
	EXPECT_EQ(1, palette.size()) << palette;
}

// https://github.com/vengi-voxel/vengi/issues/418
TEST_F(SceneManagerTest, testDuplicateAndRemove) {
	// prepare scenegraph with multiple nodes and a reference
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_EQ(2, _sceneMgr->sceneGraph().nodeSize());
	const int cnodeId = _sceneMgr->addModelChild("children", 1, 1, 1);
	ASSERT_NE(cnodeId, InvalidNodeId);
	const int crnodeId = _sceneMgr->nodeReference(cnodeId);
	ASSERT_NE(crnodeId, InvalidNodeId);
	ASSERT_EQ(4, _sceneMgr->sceneGraph().nodeSize());

	int newNodeId = InvalidNodeId;
	ASSERT_TRUE(_sceneMgr->nodeDuplicate(nodeId, &newNodeId));
	ASSERT_NE(newNodeId, InvalidNodeId);
	ASSERT_EQ(7, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_TRUE(_sceneMgr->nodeRemove(newNodeId, true));
	ASSERT_EQ(4, _sceneMgr->sceneGraph().nodeSize());
}

TEST_F(SceneManagerTest, testDuplicateAndRemoveChild) {
	// prepare scenegraph with multiple nodes and a reference
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_EQ(2, _sceneMgr->sceneGraph().nodeSize());
	const int cnodeId = _sceneMgr->addModelChild("children", 1, 1, 1);
	ASSERT_NE(cnodeId, InvalidNodeId);
	const int crnodeId = _sceneMgr->nodeReference(cnodeId);
	ASSERT_NE(crnodeId, InvalidNodeId);
	_sceneMgr->nodeReference(cnodeId);
	ASSERT_EQ(5, _sceneMgr->sceneGraph().nodeSize());

	int newNodeId = InvalidNodeId;
	ASSERT_TRUE(_sceneMgr->nodeDuplicate(nodeId, &newNodeId));
	ASSERT_NE(newNodeId, InvalidNodeId);
	ASSERT_EQ(9, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_TRUE(_sceneMgr->nodeRemove(cnodeId, true));
	ASSERT_EQ(4, _sceneMgr->sceneGraph().nodeSize());
}

// https://github.com/vengi-voxel/vengi/issues/425
TEST_F(SceneManagerTest, testUnReferenceAndUndo) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const voxel::RawVolume *v1 = _sceneMgr->volume(nodeId);
	const int rnodeId = _sceneMgr->nodeReference(nodeId);
	ASSERT_NE(rnodeId, InvalidNodeId);
	ASSERT_EQ(3, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_EQ(1, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	EXPECT_TRUE(_sceneMgr->nodeUnreference(rnodeId));
	ASSERT_EQ(2, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	ASSERT_NE(v1, _sceneMgr->volume(rnodeId));
	EXPECT_TRUE(_sceneMgr->undo());
	ASSERT_EQ(3, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_EQ(1, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	ASSERT_EQ(v1, _sceneMgr->volume(rnodeId));
	EXPECT_TRUE(_sceneMgr->redo());
}

// https://github.com/vengi-voxel/vengi/issues/425
// the difference here to testUnReferenceAndUndo() is that the previous created different memento states
// while doing all the actions to got to the state the bug was triggered - this one in turn is importing
// a scene with a reference node and the first action is to unref it
TEST_F(SceneManagerTest, testUnReferenceAndUndoForLoadedScene) {
	voxel::RawVolume v(voxel::Region(0, 0));
	int modelNodeId = InvalidNodeId;
	int referenceNodeId = InvalidNodeId;
	{
		scenegraph::SceneGraph sceneGraph;
		{
			scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
			model.setVolume(&v, false);
			modelNodeId = sceneGraph.emplace(core::move(model));
			ASSERT_NE(modelNodeId, InvalidNodeId);
		}
		{
			scenegraph::SceneGraphNode reference(scenegraph::SceneGraphNodeType::ModelReference);
			reference.setReference(modelNodeId);
			referenceNodeId = sceneGraph.emplace(core::move(reference));
			ASSERT_NE(referenceNodeId, InvalidNodeId);
		}
		sceneGraph.updateTransforms();
		ASSERT_TRUE(sceneMgr()->loadForTest(core::move(sceneGraph)));
	}

	ASSERT_EQ(1, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	ASSERT_EQ(3, _sceneMgr->sceneGraph().nodeSize());
	EXPECT_TRUE(_sceneMgr->nodeUnreference(referenceNodeId));
	ASSERT_EQ(2, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	ASSERT_EQ(3, _sceneMgr->sceneGraph().nodeSize());
	EXPECT_TRUE(_sceneMgr->undo());
	EXPECT_TRUE(_sceneMgr->redo());
}

TEST_F(SceneManagerTest, testChangePivotOfParentThenUndo) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_EQ(2, _sceneMgr->sceneGraph().nodeSize());
	const int cnodeId = _sceneMgr->addModelChild("children", 1, 1, 1);
	ASSERT_NE(cnodeId, InvalidNodeId);
	const glm::vec3 clocalTranslationVec = glm::vec3(10.0f);
	const glm::vec3 cworldTranslationFinal = glm::vec3(12.0f);
	const scenegraph::KeyFrameIndex keyFrameIndex = 0;
	const glm::mat4 ctranslationMat = glm::translate(clocalTranslationVec);
	scenegraph::SceneGraphNode *cnode = _sceneMgr->sceneGraphNode(cnodeId);
	ASSERT_NE(cnode, nullptr);
	EXPECT_EQ(cnode->region().getDimensionsInVoxels(), glm::ivec3(1));
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId);
	ASSERT_NE(node, nullptr);
	{
		const scenegraph::SceneGraphTransform &ctransform = cnode->transform(keyFrameIndex);
		EXPECT_EQ(node->region().getDimensionsInVoxels(), glm::ivec3(2));

		ASSERT_TRUE(_sceneMgr->nodeUpdateTransform(cnodeId, ctranslationMat, keyFrameIndex, false));
		ASSERT_VEC_NEAR(ctransform.localTranslation(), clocalTranslationVec, 0.0001f);
		ASSERT_VEC_NEAR(ctransform.worldTranslation(), ctransform.localTranslation(), 0.0001f)
			<< "local and world should match at this point";
		ASSERT_TRUE(_sceneMgr->nodeUpdatePivot(nodeId, glm::vec3(1.0f, 1.0f, 1.0f)));
		ASSERT_VEC_NEAR(ctransform.localTranslation(), clocalTranslationVec, 0.0001f);
		ASSERT_VEC_NEAR(ctransform.worldTranslation(), cworldTranslationFinal, 0.0001f);
	}
	ASSERT_TRUE(_sceneMgr->undo());
	{
		const glm::vec3 pivot = node->pivot();
		ASSERT_VEC_NEAR(pivot, glm::vec3(0.0f), 0.0001f);
		const scenegraph::SceneGraphTransform &ctransform = cnode->transform(keyFrameIndex);
		ASSERT_VEC_NEAR(ctransform.worldTranslation(), clocalTranslationVec, 0.0001f);
		ASSERT_VEC_NEAR(node->pivot(), glm::vec3(0.0f), 0.0001f);
	}
	ASSERT_TRUE(_sceneMgr->redo());
	{
		const scenegraph::SceneGraphTransform &ctransform = cnode->transform(keyFrameIndex);
		ASSERT_VEC_NEAR(ctransform.worldTranslation(), cworldTranslationFinal, 0.0001f);
		ASSERT_VEC_NEAR(node->pivot(), glm::vec3(1.0f), 0.0001f);
	}
}

TEST_F(SceneManagerTest, testAddAnimationThenUndo) {
	ASSERT_TRUE(_sceneMgr->addAnimation("foo"));
	EXPECT_EQ(2u, _sceneMgr->mementoHandler().stateSize());
	EXPECT_EQ(2u, _sceneMgr->sceneGraph().animations().size());
	ASSERT_TRUE(_sceneMgr->undo());
	EXPECT_EQ(1u, _sceneMgr->sceneGraph().animations().size());
}

TEST_F(SceneManagerTest, testGetSuggestedFilename) {
	EXPECT_EQ("scene.vengi", _sceneMgr->getSuggestedFilename());
	sceneMgr()->setLastFilename("test.vengi");
	EXPECT_EQ("test.vengi", _sceneMgr->getSuggestedFilename());
	EXPECT_EQ("test.png", _sceneMgr->getSuggestedFilename("png"));
	EXPECT_EQ("test.png", _sceneMgr->getSuggestedFilename(".png"));
	sceneMgr()->setLastFilename("test", &voxelformat::VoxFormat::format());
	EXPECT_EQ("test.vox", _sceneMgr->getSuggestedFilename());
	// TODO: here we need to define which extension should be used - from the format, or the given one...
	sceneMgr()->setLastFilename("test.vengi", &voxelformat::VoxFormat::format());
	EXPECT_EQ("test.vengi", _sceneMgr->getSuggestedFilename());
	{
		const core::String fullPath = core::string::path("path", "to", "scenefull.vengi");
		sceneMgr()->setLastFilename(fullPath);
		EXPECT_EQ(fullPath, _sceneMgr->getSuggestedFilename());
	}
}

TEST_F(SceneManagerTest, testReduceColors) {
	Modifier &modifier = _sceneMgr->modifier();
	const voxel::Region region{0, 5};
	ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));

	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	core::Buffer<uint8_t> srcBuf;
	voxel::Voxel targetVoxel = modifier.cursorVoxel();
	for (int i = 0; i < 4; ++i) {
		v->setVoxel(glm::ivec3(i, 1, 1), voxel::createVoxel(voxel::VoxelType::Generic, i));
		v->setVoxel(glm::ivec3(i, i, i), voxel::createVoxel(voxel::VoxelType::Generic, i));
		if (i != targetVoxel.getColor()) {
			srcBuf.push_back(i);
		}
	}
	EXPECT_EQ(2, countVoxels(*v, targetVoxel));
	EXPECT_TRUE(_sceneMgr->nodeReduceColors(nodeId, srcBuf, targetVoxel.getColor()));
	EXPECT_EQ(7, countVoxels(*v, targetVoxel));
}

TEST_F(SceneManagerTest, testRemoveColors) {
	Modifier &modifier = _sceneMgr->modifier();
	const voxel::Region region{0, 0};

	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	voxel::Voxel targetVoxel = modifier.cursorVoxel();
	v->setVoxel(0, 0, 0, targetVoxel);
	EXPECT_EQ(targetVoxel.getColor(), v->voxel(0, 0, 0).getColor());
	EXPECT_TRUE(_sceneMgr->nodeRemoveColor(nodeId, targetVoxel.getColor()));
	EXPECT_NE(targetVoxel.getColor(), v->voxel(0, 0, 0).getColor());
	EXPECT_TRUE(voxel::isBlocked(v->voxel(0, 0, 0).getMaterial()));
}

} // namespace voxedit
