/**
 * @file
 */

#include "../SceneManager.h"
#include "../Config.h"
#include "app/tests/AbstractTest.h"
#include "core/TimeProvider.h"
#include "math/tests/TestMathHelper.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelutil/VolumeVisitor.h"

namespace palette {

::std::ostream &operator<<(::std::ostream &os, const Palette &palette) {
	return os << Palette::print(palette).c_str();
}

}

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
};

class SceneManagerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	SceneManagerPtr _sceneMgr;

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
		core::Var::get(cfg::VoxelMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Cubic));
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
		modifier.setBrushType(BrushType::None);
		modifier.setModifierType(ModifierType::Select);
		modifier.setCursorPosition(mins, voxel::FaceNames::NegativeX);
		EXPECT_TRUE(modifier.start());
		modifier.setCursorPosition(maxs, voxel::FaceNames::NegativeX);
		modifier.executeAdditionalAction();
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		EXPECT_TRUE(modifier.execute(sceneGraph, node, [&](const voxel::Region &, ModifierType, bool) {}));
		modifier.setBrushType(BrushType::Shape);
		modifier.setModifierType(ModifierType::Place);
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
	EXPECT_NE(-1, _sceneMgr->addModelChild("second node", 1, 1, 1));
	EXPECT_NE(-1, _sceneMgr->addModelChild("third node", 1, 1, 1));
	EXPECT_EQ(3u, _sceneMgr->mementoHandler().stateSize());

	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_EQ(3u, _sceneMgr->sceneGraph().size());

	for (int i = 0; i < 3; ++i) {
		SCOPED_TRACE(i);
		{
			EXPECT_EQ(2u, mementoHandler.statePosition());
			ASSERT_TRUE(mementoHandler.canUndo());
			EXPECT_TRUE(_sceneMgr->undo());
			EXPECT_EQ(1u, mementoHandler.statePosition());
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(2u, _sceneMgr->sceneGraph().size());
		}
		{
			EXPECT_TRUE(_sceneMgr->undo());
			ASSERT_FALSE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(1u, _sceneMgr->sceneGraph().size());
		}
		{
			EXPECT_TRUE(_sceneMgr->redo());
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(mementoHandler.canRedo());
			EXPECT_EQ(2u, _sceneMgr->sceneGraph().size());
		}
		{
			EXPECT_TRUE(_sceneMgr->redo());
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_FALSE(mementoHandler.canRedo());
			EXPECT_EQ(3u, _sceneMgr->sceneGraph().size());
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
			EXPECT_EQ(2u, _sceneMgr->sceneGraph().size());
			ASSERT_TRUE(mementoHandler.canUndo());
			ASSERT_TRUE(_sceneMgr->undo());
			EXPECT_EQ(1u, _sceneMgr->sceneGraph().size());
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
	EXPECT_FALSE(modifier.selections().empty());
	EXPECT_TRUE(_sceneMgr->copy());

	EXPECT_NE(-1, _sceneMgr->addModelChild("paste target", 1, 1, 1));
	EXPECT_TRUE(_sceneMgr->paste(testMins()));
	EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
}

TEST_F(SceneManagerTest, testMergeSimple) {
	Modifier &modifier = _sceneMgr->modifier();
	int secondNodeId = _sceneMgr->addModelChild("second node", 10, 10, 10);
	int thirdNodeId = _sceneMgr->addModelChild("third node", 10, 10, 10);
	ASSERT_NE(-1, secondNodeId);
	ASSERT_NE(-1, thirdNodeId);

	// set voxel into second node
	EXPECT_TRUE(_sceneMgr->nodeActivate(secondNodeId));
	testSetVoxel(glm::ivec3(1, 1, 1));
	EXPECT_EQ(1, countVoxels(*_sceneMgr->volume(secondNodeId), modifier.cursorVoxel()));

	// set voxel into third node
	EXPECT_TRUE(_sceneMgr->nodeActivate(thirdNodeId));
	testSetVoxel(glm::ivec3(2, 2, 2));
	EXPECT_EQ(1, countVoxels(*_sceneMgr->volume(thirdNodeId), modifier.cursorVoxel()));

	// merge and validate
	int newNodeId = _sceneMgr->mergeNodes(secondNodeId, thirdNodeId);
	const voxel::RawVolume *v = _sceneMgr->volume(newNodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(2, countVoxels(*v, modifier.cursorVoxel()));
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
	ASSERT_EQ(1, _sceneMgr->sceneGraph().size());
	EXPECT_TRUE(_sceneMgr->nodeUnreference(rnodeId));
	ASSERT_EQ(2, _sceneMgr->sceneGraph().size());
	ASSERT_NE(v1, _sceneMgr->volume(rnodeId));
	EXPECT_TRUE(_sceneMgr->undo());
	ASSERT_EQ(3, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_EQ(1, _sceneMgr->sceneGraph().size());
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
		ASSERT_TRUE(((SceneManagerEx*)_sceneMgr.get())->loadForTest(core::move(sceneGraph)));
	}

	ASSERT_EQ(1, _sceneMgr->sceneGraph().size());
	ASSERT_EQ(3, _sceneMgr->sceneGraph().nodeSize());
	EXPECT_TRUE(_sceneMgr->nodeUnreference(referenceNodeId));
	ASSERT_EQ(2, _sceneMgr->sceneGraph().size());
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
	glm::vec3 cworldTranslation;
	{
		const scenegraph::SceneGraphTransform &ctransform = cnode->transform(keyFrameIndex);
		EXPECT_EQ(node->region().getDimensionsInVoxels(), glm::ivec3(2));

		ASSERT_TRUE(_sceneMgr->nodeUpdateTransform(cnodeId, ctranslationMat, keyFrameIndex, false));
		cworldTranslation = ctransform.worldTranslation();
		ASSERT_VEC_NEAR(ctransform.localTranslation(), clocalTranslationVec, 0.0001f);
		ASSERT_VEC_NEAR(cworldTranslation, clocalTranslationVec, 0.0001f);
		ASSERT_TRUE(_sceneMgr->nodeUpdatePivot(nodeId, glm::vec3(1.0f, 1.0f, 1.0f)));
		ASSERT_VEC_NEAR(ctransform.localTranslation(), clocalTranslationVec, 0.0001f);
		ASSERT_VEC_NEAR(ctransform.worldTranslation(), cworldTranslationFinal, 0.0001f);
	}
	ASSERT_TRUE(_sceneMgr->undo());
	{
		const scenegraph::SceneGraphTransform &ctransform = cnode->transform(keyFrameIndex);
		ASSERT_VEC_NEAR(ctransform.worldTranslation(), cworldTranslation, 0.0001f);
		ASSERT_VEC_NEAR(node->pivot(), glm::vec3(0.0f), 0.0001f);
	}
	ASSERT_TRUE(_sceneMgr->redo());
	{
		const scenegraph::SceneGraphTransform &ctransform = cnode->transform(keyFrameIndex);
		ASSERT_VEC_NEAR(ctransform.worldTranslation(), cworldTranslationFinal, 0.0001f);
		ASSERT_VEC_NEAR(node->pivot(), glm::vec3(1.0f), 0.0001f);
	}
}

} // namespace voxedit
