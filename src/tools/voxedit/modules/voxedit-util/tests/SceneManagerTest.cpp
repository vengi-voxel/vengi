/**
 * @file
 */

#include "voxedit-util/SceneManager.h"
#include "voxedit-util/Config.h"
#include "AbstractSceneManagerTest.h"
#include "image/Image.h"
#include "io/FilesystemArchive.h"
#include "math/tests/TestMathHelper.h"
#include "palette/Palette.h"
#include "palette/tests/TestHelper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/tests/TestHelper.h"
#include "util/VarUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

class SceneManagerTest : public AbstractSceneManagerTest {
private:
	using Super = AbstractSceneManagerTest;

protected:
	void SetUp() override {
		Super::SetUp();

		Modifier &modifier = _sceneMgr->modifier();
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setBrushType(BrushType::Shape);
		modifier.setModifierType(ModifierType::Place);
		memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
	}

	void loadVengiFile(const core::String &filename) {
		voxelformat::VENGIFormat format;
		const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem());
		io::FileDescription fileDesc;
		fileDesc.set(filename);
		scenegraph::SceneGraph newSceneGraph;
		voxelformat::LoadContext testLoadCtx;
		ASSERT_TRUE(loadFormat(fileDesc, archive, newSceneGraph, testLoadCtx)) << "Failed to load " << filename;
		ASSERT_TRUE(_sceneMgr->loadSceneGraph(core::move(newSceneGraph)));
	}

	bool testSetVoxel(const glm::ivec3 &pos, int paletteColorIndex = 1) {
		Modifier &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::Shape);
		modifier.shapeBrush().setSingleMode();
		modifier.setModifierType(ModifierType::Override);
		modifier.setCursorPosition(pos, voxel::FaceNames::NegativeX);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, paletteColorIndex));
		if (!modifier.beginBrush()) {
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
		auto callback = [&](const voxel::Region &region, ModifierType, SceneModifiedFlags) {
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
		modifier.endBrush();
		modifier.setBrushType(BrushType::Select);
		modifier.setCursorPosition(mins, voxel::FaceNames::NegativeX);
		EXPECT_TRUE(modifier.beginBrush());
		modifier.setCursorPosition(maxs, voxel::FaceNames::NegativeX);
		modifier.executeAdditionalAction();
		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(sceneGraph.activeNode());
		ASSERT_NE(nullptr, node);
		EXPECT_TRUE(
			modifier.execute(sceneGraph, *node, [&](const voxel::Region &, ModifierType, SceneModifiedFlags) {}));
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
	testSetVoxel(testMins(), 1);
	testSelect(testMins(), testMaxs());
	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
	ASSERT_NE(nullptr, node);
	EXPECT_TRUE(node->hasSelection());
	EXPECT_TRUE(_sceneMgr->copy(node->id()));

	EXPECT_NE(-1, _sceneMgr->addModelChild("paste target", 1, 1, 1));
	EXPECT_TRUE(_sceneMgr->paste(testMins()));
	EXPECT_EQ(1, testVolume()->voxel(0, 0, 0).getColor());
}

TEST_F(SceneManagerTest, testExceedsMaxSuggestedVolumeSize) {
	util::ScopedVarChange scoped(cfg::VoxEditMaxSuggestedVolumeSize, "32");
	const voxel::Region region{0, 33};
	ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));
	ASSERT_TRUE(_sceneMgr->exceedsMaxSuggestedVolumeSize());

	const voxel::Region regionSmall{0, 15};
	ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", regionSmall));
	ASSERT_FALSE(_sceneMgr->exceedsMaxSuggestedVolumeSize());
}

TEST_F(SceneManagerTest, testMergeVengiFile) {
	loadVengiFile("test-merge.vengi");
	if (HasFailure()) {
		return;
	}

	ASSERT_EQ(2u, _sceneMgr->sceneGraph().size());
	_sceneMgr->mergeNodes(NodeMergeFlags::All);
	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size());

	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *v = _sceneMgr->volume(node->id());
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(16, voxelutil::countVoxels(*v));
	const voxel::Region region(-2, 0, 0, 3, 1, 1);
	EXPECT_EQ(region.getDimensionsInVoxels(), v->region().getDimensionsInVoxels());
	EXPECT_EQ(region.getLowerCorner(), v->region().getLowerCorner());
	const scenegraph::SceneGraphTransform &transform = node->transform(0);
	EXPECT_EQ(transform.localTranslation(), glm::vec3(0.0f));
	EXPECT_EQ(transform.localScale(), glm::vec3(1.0f));
}

TEST_F(SceneManagerTest, testMerge2VengiFile) {
	loadVengiFile("test-merge2.vengi");
	if (HasFailure()) {
		return;
	}

	ASSERT_EQ(3u, _sceneMgr->sceneGraph().size());
	_sceneMgr->mergeNodes(NodeMergeFlags::All);
	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size());

	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *v = _sceneMgr->volume(node->id());
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(132, voxelutil::countVoxels(*v));
	const voxel::Region region(0, 0, -1, 3, 9, 5);
	EXPECT_EQ(region.getDimensionsInVoxels(), v->region().getDimensionsInVoxels());
	EXPECT_EQ(region.getLowerCorner(), v->region().getLowerCorner());
	const scenegraph::SceneGraphTransform &transform = node->transform(0);
	EXPECT_EQ(transform.localTranslation(), glm::vec3(0.0f));
	EXPECT_EQ(transform.localScale(), glm::vec3(1.0f));
}

TEST_F(SceneManagerTest, testMerge2VengiFileBakeTransform) {
	loadVengiFile("test-merge2.vengi");
	if (HasFailure()) {
		return;
	}

	ASSERT_EQ(3u, _sceneMgr->sceneGraph().size());
	{
		SCOPED_TRACE("K_Foot_Right");
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByName("K_Foot_Right");
		ASSERT_NE(nullptr, node);
		ASSERT_TRUE(node->isModelNode());
		const voxel::Region originalRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(originalRegion, voxel::Region(0, 0, 0, 3, 5, 3));
		_sceneMgr->nodeBakeTransform(node->id());
		const voxel::Region newRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(newRegion, voxel::Region(0, 0, -1, 3, 5, 2));
		EXPECT_VEC_NEAR(glm::vec3(0.0f, 0.0f, 0.0f), node->transform().worldTranslation(), 0.0001f);
	}
	{
		SCOPED_TRACE("K_Toe_Right");
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByName("K_Toe_Right");
		ASSERT_NE(nullptr, node);
		ASSERT_TRUE(node->isModelNode());
		const voxel::Region originalRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(originalRegion, voxel::Region(0, 0, 0, 3, 2, 2));
		_sceneMgr->nodeBakeTransform(node->id());
		const voxel::Region newRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(newRegion, voxel::Region(0, 0, 3, 3, 2, 5));
		EXPECT_VEC_NEAR(glm::vec3(0.0f, 0.0f, 0.0f), node->transform().worldTranslation(), 0.0001f);
	}
	{
		SCOPED_TRACE("K_Leg_Right_l");
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByName("K_Leg_Right_l");
		ASSERT_NE(nullptr, node);
		ASSERT_TRUE(node->isModelNode());
		const voxel::Region originalRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(originalRegion, voxel::Region(0, 0, 0, 1, 5, 1));
		_sceneMgr->nodeBakeTransform(node->id());
		const voxel::Region newRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(newRegion, voxel::Region(1, 4, 0, 2, 9, 1));
		EXPECT_VEC_NEAR(glm::vec3(0.0f, 0.0f, 0.0f), node->transform().worldTranslation(), 0.0001f);
	}
}

TEST_F(SceneManagerTest, DISABLED_testChrKnightBakeTransform) {
	loadVengiFile("chr_knight.vengi");
	if (HasFailure()) {
		return;
	}

	{
		// TODO: this is failing because the root node has 180 degree rotation on Y axis - and this is not handled
		// properly handled yet in nodeBakeTransform or applyTransformToVolume and is producing an off-by-one error on
		// the z axis. The test is correct, the implementation needs to be fixed.
		SCOPED_TRACE("K_Waist");
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByName("K_Waist");
		ASSERT_NE(nullptr, node);
		ASSERT_TRUE(node->isModelNode());
		const voxel::Region originalRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(originalRegion, voxel::Region(0, 0, 0, 8, 3, 6));
		_sceneMgr->nodeBakeTransform(node->id());
		const voxel::Region newRegion = _sceneMgr->volume(node->id())->region();
		ASSERT_EQ(newRegion, voxel::Region(-4, 14, -4, 4, 17, 2));
		EXPECT_VEC_NEAR(glm::vec3(0.0f, 0.0f, 0.0f), node->transform().worldTranslation(), 0.0001f);
	}
}

TEST_F(SceneManagerTest, testChrKnightMerge) {
	loadVengiFile("chr_knight.vengi");
	if (HasFailure()) {
		return;
	}

	{
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().firstModelNode();
		ASSERT_NE(nullptr, node);
		const voxel::RawVolume *v = _sceneMgr->volume(node->id());
		ASSERT_NE(nullptr, v);
	}

	_sceneMgr->mergeNodes(NodeMergeFlags::All);
	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size());

	{
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().firstModelNode();
		ASSERT_NE(nullptr, node);
		const voxel::RawVolume *v = _sceneMgr->volume(node->id());
		ASSERT_NE(nullptr, v);
		EXPECT_EQ(2612, voxelutil::countVoxels(*v));
	}
}

TEST_F(SceneManagerTest, testChrKnightMergeUndo) {
	loadVengiFile("chr_knight.vengi");
	if (HasFailure()) {
		return;
	}
	{
		ASSERT_EQ(19u, _sceneMgr->sceneGraph().size());
		const scenegraph::SceneGraphNode *chestBefore = _sceneMgr->sceneGraph().findNodeByName("K_Chest");
		ASSERT_NE(nullptr, chestBefore);
		const scenegraph::SceneGraphNode &coreBefore = _sceneMgr->sceneGraph().node(chestBefore->parent());
		ASSERT_EQ(coreBefore.name(), "K_Core");
	}

	_sceneMgr->mergeNodes(NodeMergeFlags::All);
	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size());
	EXPECT_TRUE(_sceneMgr->undo());

	{
		ASSERT_EQ(19u, _sceneMgr->sceneGraph().size());
		const scenegraph::SceneGraphNode *chestAfter = _sceneMgr->sceneGraph().findNodeByName("K_Chest");
		ASSERT_NE(nullptr, chestAfter) << "Chest node should be back after undo";
		ASSERT_NE(chestAfter->parent(), 0) << "Parent should not be the root node, but K_Core";
		const scenegraph::SceneGraphNode &coreAfter = _sceneMgr->sceneGraph().node(chestAfter->parent());
		ASSERT_EQ(coreAfter.name(), "K_Core") << "Parent should be K_Core after undo";
	}

	EXPECT_TRUE(_sceneMgr->redo());
	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size());
	EXPECT_TRUE(_sceneMgr->undo());
	ASSERT_EQ(19u, _sceneMgr->sceneGraph().size());
}

TEST_F(SceneManagerTest, testChrKnightMergeCoverAndHead) {
	loadVengiFile("chr_knight.vengi");
	if (HasFailure()) {
		return;
	}

	ASSERT_EQ(19u, _sceneMgr->sceneGraph().size());

	scenegraph::SceneGraphTransform &transform = _sceneMgr->sceneGraph().node(0).transform(0);
	transform.setWorldOrientation(glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f));
	_sceneMgr->sceneGraph().updateTransforms();

	const scenegraph::SceneGraphNode *head = _sceneMgr->sceneGraph().findNodeByName("K_Head");
	ASSERT_NE(nullptr, head);

	const voxel::RawVolume *headVolume = _sceneMgr->volume(head->id());
	ASSERT_NE(nullptr, headVolume);
	const voxel::Region headRegion = headVolume->region();
	EXPECT_EQ(781, voxelutil::countVoxels(*headVolume));

	const scenegraph::SceneGraphNode *cover = _sceneMgr->sceneGraph().findNodeByName("K_Cover");
	ASSERT_NE(nullptr, cover);
	const voxel::RawVolume *coverVolume = _sceneMgr->volume(cover->id());
	ASSERT_NE(nullptr, coverVolume);
	const voxel::Region coverRegion = coverVolume->region();
	EXPECT_EQ(95, voxelutil::countVoxels(*coverVolume));

	_sceneMgr->mergeNodes(head->id(), cover->id());
	ASSERT_EQ(18u, _sceneMgr->sceneGraph().size());

	const scenegraph::SceneGraphNode *mergedHead = _sceneMgr->sceneGraph().findNodeByName("K_Head");
	ASSERT_NE(nullptr, mergedHead);
	const voxel::RawVolume *mergedHeadVolume = _sceneMgr->volume(mergedHead->id());
	ASSERT_NE(nullptr, mergedHeadVolume);
	const voxel::Region &mergedRegion = mergedHeadVolume->region();

	glm::ivec3 expectedDimensions(9, 14, 13);

	EXPECT_EQ(expectedDimensions, mergedRegion.getDimensionsInVoxels())
		<< "Original head: " << headRegion.toString() << " Original cover: " << coverRegion.toString();
	// TODO: check the mins of the region here - doesn't currently work
	EXPECT_EQ(876, voxelutil::countVoxels(*mergedHeadVolume));
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
	EXPECT_EQ(1, voxelutil::countVoxelsByColor(*_sceneMgr->volume(secondNodeId), modifier.cursorVoxel()));

	// set voxel into third node
	EXPECT_TRUE(_sceneMgr->nodeActivate(thirdNodeId));
	testSetVoxel(glm::ivec3(2, 2, 2), modifier.cursorVoxel().getColor());
	EXPECT_EQ(1, voxelutil::countVoxelsByColor(*_sceneMgr->volume(thirdNodeId), modifier.cursorVoxel()));

	// merge and validate
	int newNodeId = _sceneMgr->mergeNodes(secondNodeId, thirdNodeId);
	const voxel::RawVolume *v = _sceneMgr->volume(newNodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(2, voxelutil::countVoxels(*v));
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
	EXPECT_EQ(1u, palette.size()) << palette;
}

// https://github.com/vengi-voxel/vengi/issues/418
TEST_F(SceneManagerTest, testDuplicateAndRemove) {
	// prepare scenegraph with multiple nodes and a reference
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_EQ(2u, _sceneMgr->sceneGraph().nodeSize());
	const int cnodeId = _sceneMgr->addModelChild("children", 1, 1, 1);
	ASSERT_NE(cnodeId, InvalidNodeId);
	const int crnodeId = _sceneMgr->nodeReference(cnodeId);
	ASSERT_NE(crnodeId, InvalidNodeId);
	ASSERT_EQ(4u, _sceneMgr->sceneGraph().nodeSize());

	int newNodeId = InvalidNodeId;
	ASSERT_TRUE(_sceneMgr->nodeDuplicate(nodeId, &newNodeId));
	ASSERT_NE(newNodeId, InvalidNodeId);
	ASSERT_EQ(7u, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_TRUE(_sceneMgr->nodeRemove(newNodeId, true));
	ASSERT_EQ(4u, _sceneMgr->sceneGraph().nodeSize());
}

TEST_F(SceneManagerTest, testDuplicateAndRemoveChild) {
	// prepare scenegraph with multiple nodes and a reference
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_EQ(2u, _sceneMgr->sceneGraph().nodeSize());
	const int cnodeId = _sceneMgr->addModelChild("children", 1, 1, 1);
	ASSERT_NE(cnodeId, InvalidNodeId);
	const int crnodeId = _sceneMgr->nodeReference(cnodeId);
	ASSERT_NE(crnodeId, InvalidNodeId);
	_sceneMgr->nodeReference(cnodeId);
	ASSERT_EQ(5u, _sceneMgr->sceneGraph().nodeSize());

	int newNodeId = InvalidNodeId;
	ASSERT_TRUE(_sceneMgr->nodeDuplicate(nodeId, &newNodeId));
	ASSERT_NE(newNodeId, InvalidNodeId);
	ASSERT_EQ(9u, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_TRUE(_sceneMgr->nodeRemove(cnodeId, true));
	ASSERT_EQ(4u, _sceneMgr->sceneGraph().nodeSize());
}

// https://github.com/vengi-voxel/vengi/issues/425
TEST_F(SceneManagerTest, testUnReferenceAndUndo) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const voxel::RawVolume *v1 = _sceneMgr->volume(nodeId);
	const int rnodeId = _sceneMgr->nodeReference(nodeId);
	ASSERT_NE(rnodeId, InvalidNodeId);
	ASSERT_EQ(3u, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	EXPECT_TRUE(_sceneMgr->nodeUnreference(rnodeId));
	ASSERT_EQ(2u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	ASSERT_NE(v1, _sceneMgr->volume(rnodeId));
	EXPECT_TRUE(_sceneMgr->undo());
	ASSERT_EQ(3u, _sceneMgr->sceneGraph().nodeSize());
	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
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

	ASSERT_EQ(1u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	ASSERT_EQ(3u, _sceneMgr->sceneGraph().nodeSize());
	EXPECT_TRUE(_sceneMgr->nodeUnreference(referenceNodeId));
	ASSERT_EQ(2u, _sceneMgr->sceneGraph().size()) << _sceneMgr->sceneGraph();
	ASSERT_EQ(3u, _sceneMgr->sceneGraph().nodeSize());
	EXPECT_TRUE(_sceneMgr->undo());
	EXPECT_TRUE(_sceneMgr->redo());
}

TEST_F(SceneManagerTest, testChangePivotOfParentThenUndo) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_EQ(2u, _sceneMgr->sceneGraph().nodeSize());
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
	EXPECT_EQ(2, voxelutil::countVoxelsByColor(*v, targetVoxel));
	EXPECT_TRUE(_sceneMgr->nodeReduceColors(nodeId, srcBuf, targetVoxel.getColor()));
	EXPECT_EQ(7, voxelutil::countVoxelsByColor(*v, targetVoxel));
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

TEST_F(SceneManagerTest, testMouseRayTrace) {
	const voxel::Region region{-10, 10};
	ASSERT_TRUE(_sceneMgr->newScene(true, "raycast_test", region));

	const glm::ivec3 targetPos(0, 0, 0);
	ASSERT_TRUE(testSetVoxel(targetPos, 1));

	// Create and set up a camera for the raycast
	video::Camera camera;
	camera.setNearPlane(0.1f);
	camera.setFarPlane(20.0f);
	camera.setFieldOfView(45.0f);
	camera.setSize(glm::ivec2(800, 600));

	// Position camera to look directly at the origin from positive Z
	const glm::vec3 cameraPos(0.0f, 0.0f, 10.0f);
	const glm::vec3 lookAt(0.0f, 0.0f, 0.0f);
	const glm::vec3 up(0.0f, 1.0f, 0.0f);
	camera.setWorldPosition(cameraPos);
	camera.lookAt(lookAt, up);
	camera.update(0.0);

	// Set this camera as the active camera
	_sceneMgr->setActiveCamera(&camera, false);
	ASSERT_NE(nullptr, _sceneMgr->activeCamera());

	// Set mouse position to center of screen
	_sceneMgr->setMousePos(camera.size().x / 2, camera.size().y / 2);

	// Perform the raycast with identity transformation matrix
	const glm::mat4 invModel(1.0f);
	EXPECT_TRUE(sceneMgr()->testMouseRayTrace(true, invModel));

	// Check the raycast result - we should at least get a result
	const voxelutil::PickResult &result = sceneMgr()->getPickResult();
	EXPECT_TRUE(result.firstValidPosition) << "Should have a valid first position";

	// If we hit something, validate it's reasonable
	if (result.didHit) {
		EXPECT_TRUE(result.validPreviousPosition) << "Should have a valid previous position";
		EXPECT_NE(voxel::FaceNames::Max, result.hitFace) << "Should detect which face was hit";
		// The hit position should be within our volume region
		EXPECT_TRUE(region.containsPoint(result.hitVoxel)) << "Hit voxel should be within volume region";
	}

	// Test raycast against empty space by pointing camera away from any voxels
	const glm::vec3 emptyCameraPos(0.0f, 0.0f, -10.0f); // Point away from voxels
	const glm::vec3 emptyLookAt(0.0f, 0.0f, -20.0f);
	camera.setWorldPosition(emptyCameraPos);
	camera.lookAt(emptyLookAt, up);
	camera.update(0.0);

	SceneManagerEx *sm = sceneMgr();
	EXPECT_TRUE(sm->testMouseRayTrace(true, invModel));

	// Test force parameter by calling twice with same mouse position
	sm->setMousePos(300, 300);
	EXPECT_TRUE(sm->testMouseRayTrace(false, invModel));
	EXPECT_FALSE(sm->testMouseRayTrace(false, invModel))
		<< "Without force, should not re-execute if mouse hasn't moved";
	EXPECT_TRUE(sm->testMouseRayTrace(true, invModel)) << "Force should always re-execute";

	// Test different mouse positions
	_sceneMgr->setMousePos(100, 100); // Top-left corner
	EXPECT_TRUE(sm->testMouseRayTrace(true, invModel));

	_sceneMgr->setMousePos(700, 500); // Bottom-right corner
	EXPECT_TRUE(sm->testMouseRayTrace(true, invModel));

	// Clean up
	_sceneMgr->setActiveCamera(nullptr, false);
}

TEST_F(SceneManagerTest, testColorToNewNode) {
	const voxel::Region region{0, 5};
	ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));

	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	for (int i = 0; i < 4; ++i) {
		v->setVoxel(glm::ivec3(i, 1, 1), voxel::createVoxel(voxel::VoxelType::Generic, i));
	}
	voxel::Voxel targetVoxel(voxel::VoxelType::Generic, 1);
	EXPECT_EQ(1, voxelutil::countVoxelsByColor(*v, targetVoxel));
	const int newNodeId = sceneMgr()->nodeColorToNewNode(nodeId, targetVoxel);
	EXPECT_NE(InvalidNodeId, newNodeId);
	voxel::RawVolume *newV = _sceneMgr->volume(newNodeId);
	ASSERT_NE(nullptr, newV);
	EXPECT_EQ(1, voxelutil::countVoxelsByColor(*newV, targetVoxel));
	EXPECT_EQ(0, voxelutil::countVoxelsByColor(*v, targetVoxel));
}

TEST_F(SceneManagerTest, testNodeShiftAllKeyframes) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
	const int nodeId = _sceneMgr->sceneGraph().emplace(core::move(node));
	scenegraph::SceneGraphNode &n = _sceneMgr->sceneGraph().node(nodeId);

	// validate initial state
	const scenegraph::FrameTransform &ft0 = _sceneMgr->sceneGraph().transformForFrame(n, 0);
	EXPECT_VEC_NEAR(ft0.worldTranslation(), glm::vec3(0.0f, 0.0f, 0.0f), 0.001f);

	// perform action
	const glm::vec3 shift(5.0f, 5.0f, 5.0f);
	EXPECT_TRUE(_sceneMgr->nodeShiftAllKeyframes(nodeId, shift));

	// validate shifted state
	const scenegraph::FrameTransform &ft1 = _sceneMgr->sceneGraph().transformForFrame(n, 0);
	EXPECT_VEC_NEAR(ft1.worldTranslation(), glm::vec3(5.0f, 5.0f, 5.0f), 0.001f);

	const scenegraph::FrameTransform &ft2 = _sceneMgr->sceneGraph().transformForFrame(n, 10);
	EXPECT_VEC_NEAR(ft2.worldTranslation(), glm::vec3(5.0f, 5.0f, 5.0f), 0.001f);
}

TEST_F(SceneManagerTest, testNodeTransformMirror) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)), true);
	const int nodeId = _sceneMgr->sceneGraph().emplace(core::move(node));
	scenegraph::SceneGraphNode &n = _sceneMgr->sceneGraph().node(nodeId);

	// set initial transform
	scenegraph::SceneGraphTransform transform;
	transform.setWorldTranslation(glm::vec3(10.0f, 20.0f, 30.0f));
	n.keyFrame(0).setTransform(transform);
	_sceneMgr->sceneGraph().updateTransforms();

	// validate initial state
	const scenegraph::FrameTransform &ft0 = _sceneMgr->sceneGraph().transformForFrame(n, 0);
	EXPECT_VEC_NEAR(ft0.worldTranslation(), glm::vec3(10.0f, 20.0f, 30.0f), 0.001f);

	// perform action X
	EXPECT_TRUE(_sceneMgr->nodeTransformMirror(nodeId, 0, math::Axis::X));

	// validate mirrored state X
	const scenegraph::FrameTransform &ft1 = _sceneMgr->sceneGraph().transformForFrame(n, 0);
	EXPECT_VEC_NEAR(ft1.worldTranslation(), glm::vec3(-10.0f, 20.0f, 30.0f), 0.001f);

	// perform action XZ (mirror X and Z)
	// Current state: (-10, 20, 30)
	// Mirror XZ: X -> -X, Z -> -Z
	// Expected: (10, 20, -30)
	EXPECT_TRUE(_sceneMgr->nodeTransformMirror(nodeId, 0, math::Axis::X | math::Axis::Z));

	const scenegraph::FrameTransform &ft2 = _sceneMgr->sceneGraph().transformForFrame(n, 0);
	EXPECT_VEC_NEAR(ft2.worldTranslation(), glm::vec3(10.0f, 20.0f, -30.0f), 0.001f);
}

TEST_F(SceneManagerTest, testLoadPalette) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const palette::Palette &paletteBefore = _sceneMgr->sceneGraph().node(nodeId).palette();
	const int colorCountBefore = (int)paletteBefore.size();
	EXPECT_GT(colorCountBefore, 0);
	EXPECT_TRUE(_sceneMgr->loadPalette("built-in:nippon", false, false));
	const palette::Palette &paletteAfter = _sceneMgr->sceneGraph().node(nodeId).palette();
	EXPECT_GT((int)paletteAfter.size(), 0);
}

TEST_F(SceneManagerTest, testImportPalette) {
	EXPECT_TRUE(_sceneMgr->importPalette("rgb_small.vox", true, false));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const palette::Palette &palette = _sceneMgr->sceneGraph().node(nodeId).palette();
	EXPECT_GT((int)palette.size(), 0);
}

TEST_F(SceneManagerTest, testCalculateNormals) {
	const voxel::Region region{0, 5};
	ASSERT_TRUE(_sceneMgr->newScene(true, "normals_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_TRUE(testSetVoxel(glm::ivec3(2, 2, 2), 1));
	ASSERT_TRUE(testSetVoxel(glm::ivec3(3, 2, 2), 1));
	ASSERT_TRUE(testSetVoxel(glm::ivec3(2, 3, 2), 1));
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	// check that no normal is set yet
	for (int x = 0; x <= 5; ++x) {
		for (int y = 0; y <= 5; ++y) {
			for (int z = 0; z <= 5; ++z) {
				EXPECT_EQ(v->voxel(x, y, z).getNormal(), NO_NORMAL)
					<< "Expected no normal for voxel at (" << x << ", " << y << ", " << z << ")";
			}
		}
	}
	EXPECT_TRUE(_sceneMgr->nodeCalculateNormals(nodeId, voxel::SixConnected));
	// check that normals are set for the voxels we placed
	EXPECT_NE(v->voxel(glm::ivec3(2, 2, 2)).getNormal(), NO_NORMAL);
	EXPECT_NE(v->voxel(glm::ivec3(3, 2, 2)).getNormal(), NO_NORMAL);
	EXPECT_NE(v->voxel(glm::ivec3(2, 3, 2)).getNormal(), NO_NORMAL);
}

TEST_F(SceneManagerTest, testSaveNode) {
	ASSERT_TRUE(testSetVoxel(testMins(), 1));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const core::String file = "test-savenode.vengi";
	EXPECT_TRUE(sceneMgr()->testSaveNode(nodeId, file));
}

TEST_F(SceneManagerTest, testFillHollow) {
	const voxel::Region region{0, 5};
	ASSERT_TRUE(_sceneMgr->newScene(true, "fillhollow_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();

	// create a hollow cube shell
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	for (int x = 0; x <= 5; ++x) {
		for (int y = 0; y <= 5; ++y) {
			for (int z = 0; z <= 5; ++z) {
				if (x == 0 || x == 5 || y == 0 || y == 5 || z == 0 || z == 5) {
					v->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
				}
			}
		}
	}

	const int voxelsBefore = voxelutil::countVoxels(*v);
	sceneMgr()->testFillHollow();
	const int voxelsAfter = voxelutil::countVoxels(*v);
	EXPECT_GT(voxelsAfter, voxelsBefore) << "fillHollow should have filled the interior";
}

TEST_F(SceneManagerTest, testFill) {
	const voxel::Region region{0, 3};
	ASSERT_TRUE(_sceneMgr->newScene(true, "fill_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	EXPECT_EQ(0, voxelutil::countVoxels(*v));
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setModifierType(ModifierType::Override);
	sceneMgr()->testFill();
	const int voxelsAfter = voxelutil::countVoxels(*v);
	const int expectedVoxels = region.getWidthInVoxels() * region.getHeightInVoxels() * region.getDepthInVoxels();
	EXPECT_EQ(expectedVoxels, voxelsAfter);
}

TEST_F(SceneManagerTest, testClear) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_TRUE(testSetVoxel(testMins(), 1));
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_GT(voxelutil::countVoxels(*v), 0);

	sceneMgr()->testClear();
	EXPECT_EQ(0, voxelutil::countVoxels(*v));
}

TEST_F(SceneManagerTest, testDeleteSelected) {
	const voxel::Region region{0, 5};
	ASSERT_TRUE(_sceneMgr->newScene(true, "deleteselected_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	// Fill the entire volume
	for (int x = 0; x <= 5; ++x) {
		for (int y = 0; y <= 5; ++y) {
			for (int z = 0; z <= 5; ++z) {
				v->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	const int totalVoxels = voxelutil::countVoxels(*v);
	ASSERT_EQ(216, totalVoxels);

	// Select only a sub-region
	const voxel::Region selRegion{0, 2};
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
	ASSERT_NE(nullptr, node);
	node->select(selRegion);
	ASSERT_TRUE(node->hasSelection());

	sceneMgr()->testDeleteSelected();

	const int voxelsAfter = voxelutil::countVoxels(*v);
	const int selectedVoxels = selRegion.getWidthInVoxels() * selRegion.getHeightInVoxels() * selRegion.getDepthInVoxels();
	EXPECT_EQ(totalVoxels - selectedVoxels, voxelsAfter)
		<< "Only selected voxels should have been removed";
}

TEST_F(SceneManagerTest, testDeleteSelectedNoSelection) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_TRUE(testSetVoxel(testMins(), 1));
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	const int voxelsBefore = voxelutil::countVoxels(*v);
	EXPECT_GT(voxelsBefore, 0);

	// No selection - deleteSelected should be a no-op
	sceneMgr()->testDeleteSelected();
	EXPECT_EQ(voxelsBefore, voxelutil::countVoxels(*v));
}

TEST_F(SceneManagerTest, testHollow) {
	const voxel::Region region{0, 5};
	ASSERT_TRUE(_sceneMgr->newScene(true, "hollow_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	// fill the entire volume with voxels (solid cube)
	for (int x = 0; x <= 5; ++x) {
		for (int y = 0; y <= 5; ++y) {
			for (int z = 0; z <= 5; ++z) {
				v->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}

	const int voxelsBefore = voxelutil::countVoxels(*v);
	sceneMgr()->testHollow();
	const int voxelsAfter = voxelutil::countVoxels(*v);
	EXPECT_LT(voxelsAfter, voxelsBefore) << "hollow should have removed interior voxels";
}

TEST_F(SceneManagerTest, testFillPlane) {
	const voxel::Region region{0, 5};
	ASSERT_TRUE(_sceneMgr->newScene(true, "fillplane_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	// place a seed voxel that fillPlane will start from
	v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	EXPECT_EQ(1, voxelutil::countVoxels(*v));

	// create a small image for the fill plane operation
	image::ImagePtr img = image::createEmptyImage("fillplane");
	const uint8_t rgba[] = {255, 0, 0, 255};
	ASSERT_TRUE(img->loadRGBA(rgba, 1, 1));

	// set cursor position and face for the fill plane operation
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setCursorPosition(glm::ivec3(0, 0, 0), voxel::FaceNames::PositiveY);
	_sceneMgr->fillPlane(img);
}

TEST_F(SceneManagerTest, testNodeUpdateVoxelType) {
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	ASSERT_TRUE(testSetVoxel(testMins(), 1));
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	const voxel::Voxel &before = v->voxel(testMins());
	EXPECT_EQ(voxel::VoxelType::Generic, before.getMaterial());

	_sceneMgr->nodeUpdateVoxelType(nodeId, 1, voxel::VoxelType::Transparent);
	const voxel::Voxel &after = v->voxel(testMins());
	EXPECT_EQ(voxel::VoxelType::Transparent, after.getMaterial());
}

TEST_F(SceneManagerTest, testSaveModels) {
	ASSERT_TRUE(testSetVoxel(testMins(), 1));
	EXPECT_NE(InvalidNodeId, _sceneMgr->addModelChild("second node", 1, 1, 1));
	EXPECT_TRUE(sceneMgr()->testSaveModels("."));
}

TEST_F(SceneManagerTest, testImport) {
	const size_t nodesBefore = _sceneMgr->sceneGraph().size();
	EXPECT_TRUE(_sceneMgr->import("rgb_small.vox"));
	const size_t nodesAfter = _sceneMgr->sceneGraph().size();
	EXPECT_GT(nodesAfter, nodesBefore) << "Import should have added at least one model node";
}

TEST_F(SceneManagerTest, testSplitNodes) {
	const voxel::Region region{0, 9};
	ASSERT_TRUE(_sceneMgr->newScene(true, "split_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	// place two disconnected voxel groups
	v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	v->setVoxel(9, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	const size_t nodesBefore = _sceneMgr->sceneGraph().size();
	sceneMgr()->testSplitObjects(nodeId);
	const size_t nodesAfter = _sceneMgr->sceneGraph().size();
	EXPECT_GT(nodesAfter, nodesBefore) << "Split should have created additional nodes";
}

TEST_F(SceneManagerTest, testFlip) {
	const voxel::Region region{0, 3};
	ASSERT_TRUE(_sceneMgr->newScene(true, "flip_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	// place a voxel at one corner with a specific color
	v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	EXPECT_FALSE(voxel::isAir(v->voxel(0, 0, 0).getMaterial()));

	sceneMgr()->testFlip(math::Axis::X);

	// after flip on X axis, get the new volume (flip replaces the volume)
	v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	const int voxelCount = voxelutil::countVoxels(*v);
	EXPECT_EQ(1, voxelCount) << "Flip should preserve the voxel count";
}

TEST_F(SceneManagerTest, testNodeRotateAllNodes) {
	const voxel::Region region{0, 0, 0, 3, 3, 6};
	ASSERT_TRUE(_sceneMgr->newScene(true, "rotate_all_multi_test", region));
	const int firstNodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v1 = _sceneMgr->volume(firstNodeId);
	ASSERT_NE(nullptr, v1);
	v1->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	// Set an asymmetric pivot on the first node to verify X<->Z swap
	const glm::vec3 pivot1(0.25f, 0.0f, 0.75f);
	_sceneMgr->sceneGraph().node(firstNodeId).setPivot(pivot1);

	const int secondNodeId = _sceneMgr->addModelChild("second node", 4, 8, 4);
	ASSERT_NE(InvalidNodeId, secondNodeId);
	voxel::RawVolume *v2 = _sceneMgr->volume(secondNodeId);
	ASSERT_NE(nullptr, v2);
	v2->setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 2));

	// Set an asymmetric pivot on the second node
	const glm::vec3 pivot2(1.0f, 0.5f, 0.0f);
	_sceneMgr->sceneGraph().node(secondNodeId).setPivot(pivot2);

	_sceneMgr->sceneGraph().updateTransforms();

	sceneMgr()->testNodeRotateAll(math::Axis::Y);

	// Y-axis rotation swaps X and Z dimensions in the volumes
	// rotateAxis mapping: (x, y, z) -> (srcMaxs.z - z, y, x)

	// First node: region {0,0,0, 3,3,6} (4x4x7) -> {0,0,0, 6,3,3} (7x4x4)
	v1 = _sceneMgr->volume(firstNodeId);
	ASSERT_NE(nullptr, v1);
	EXPECT_EQ(1, voxelutil::countVoxels(*v1)) << "First node should preserve voxel count after rotation";
	EXPECT_EQ(v1->region(), (voxel::Region{0, 0, 0, 6, 3, 3})) << "First node region should have X and Z swapped";
	// Voxel (0,0,0) -> (6-0, 0, 0) = (6, 0, 0)
	EXPECT_TRUE(voxel::isBlocked(v1->voxel(6, 0, 0).getMaterial()))
		<< "Voxel should be at (6,0,0) after Y rotation";
	EXPECT_TRUE(voxel::isAir(v1->voxel(0, 0, 0).getMaterial()))
		<< "Original position (0,0,0) should be empty after rotation";

	// Second node: region {0,0,0, 3,7,3} (4x8x4) -> {0,0,0, 3,7,3} (4x8x4, same since X==Z)
	v2 = _sceneMgr->volume(secondNodeId);
	ASSERT_NE(nullptr, v2);
	EXPECT_EQ(1, voxelutil::countVoxels(*v2)) << "Second node should preserve voxel count after rotation";
	EXPECT_EQ(v2->region(), (voxel::Region{0, 0, 0, 3, 7, 3}))
		<< "Second node region should be unchanged (X and Z dims are equal)";
	// Voxel (2,2,2) -> (3-2, 2, 2) = (1, 2, 2)
	EXPECT_TRUE(voxel::isBlocked(v2->voxel(1, 2, 2).getMaterial()))
		<< "Voxel should be at (1,2,2) after Y rotation";
	EXPECT_TRUE(voxel::isAir(v2->voxel(2, 2, 2).getMaterial()))
		<< "Original position (2,2,2) should be empty after rotation";

	// Pivots: Y-axis rotation swaps X<->Z pivot components
	// pivot1 (0.25, 0.0, 0.75) -> (0.75, 0.0, 0.25)
	// pivot2 (1.0, 0.5, 0.0) -> (0.0, 0.5, 1.0)
	const scenegraph::SceneGraphNode &n1 = _sceneMgr->sceneGraph().node(firstNodeId);
	EXPECT_VEC_NEAR(n1.pivot(), glm::vec3(0.75f, 0.0f, 0.25f), 0.0001f)
		<< "First node pivot should have X and Z swapped";
	const scenegraph::SceneGraphNode &n2 = _sceneMgr->sceneGraph().node(secondNodeId);
	EXPECT_VEC_NEAR(n2.pivot(), glm::vec3(0.0f, 0.5f, 1.0f), 0.0001f)
		<< "Second node pivot should have X and Z swapped";

	// World translations after rotation around the scene pivot
	// The scene region (affected by pivots via OBB) accumulates to (-4,-4,-6) to (2,3,3)
	// Scene center (calcCenterf): (-0.5, 0.0, -1.0)
	// Node 1: worldCenter(2,2,3.5) rotated around scene center -> (-8.5, 0.0, -0.5)
	// Node 2 (child, inherits parent translation): worldCenter(-6.5,4,1.5) rotated -> (-5.0, 0.0, -9.0)
	const scenegraph::FrameTransform &ft1 = _sceneMgr->sceneGraph().transformForFrame(n1, 0);
	EXPECT_VEC_NEAR(ft1.worldTranslation(), glm::vec3(-8.5f, 0.0f, -0.5f), 0.001f)
		<< "First node world translation after rotation";

	const scenegraph::FrameTransform &ft2 = _sceneMgr->sceneGraph().transformForFrame(n2, 0);
	EXPECT_VEC_NEAR(ft2.worldTranslation(), glm::vec3(-5.0f, 0.0f, -9.0f), 0.001f)
		<< "Second node world translation after rotation";
}

TEST_F(SceneManagerTest, testNodeResizeEnlargeUndoRedo) {
	const voxel::Region region{0, 3};
	ASSERT_TRUE(_sceneMgr->newScene(true, "resize_enlarge_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	// Place a voxel to have something to verify after undo/redo
	v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	_sceneMgr->modified(nodeId, v->region());

	const voxel::Region oldRegion = v->region();
	EXPECT_EQ(oldRegion, (voxel::Region{0, 3}));

	// Enlarge the volume
	const voxel::Region newRegion{0, 7};
	_sceneMgr->nodeResize(nodeId, newRegion);

	v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(v->region(), newRegion) << "Volume should be resized to new region";
	EXPECT_TRUE(voxel::isBlocked(v->voxel(1, 1, 1).getMaterial())) << "Original voxel should be preserved";

	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();

	for (int i = 0; i < 3; ++i) {
		// Undo the resize
		ASSERT_TRUE(mementoHandler.canUndo());
		EXPECT_TRUE(_sceneMgr->undo());
		v = _sceneMgr->volume(nodeId);
		ASSERT_NE(nullptr, v);
		EXPECT_EQ(v->region(), oldRegion) << "Undo should restore original region (cycle " << i << ")";
		EXPECT_TRUE(voxel::isBlocked(v->voxel(1, 1, 1).getMaterial()))
			<< "Original voxel should exist after undo (cycle " << i << ")";

		// Redo the resize
		ASSERT_TRUE(mementoHandler.canRedo());
		EXPECT_TRUE(_sceneMgr->redo());
		v = _sceneMgr->volume(nodeId);
		ASSERT_NE(nullptr, v);
		EXPECT_EQ(v->region(), newRegion) << "Redo should restore enlarged region (cycle " << i << ")";
		EXPECT_TRUE(voxel::isBlocked(v->voxel(1, 1, 1).getMaterial()))
			<< "Original voxel should exist after redo (cycle " << i << ")";
	}
}

TEST_F(SceneManagerTest, testNodeResizeShrinkUndoRedo) {
	const voxel::Region region{0, 7};
	ASSERT_TRUE(_sceneMgr->newScene(true, "resize_shrink_test", region));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);

	// Place voxels: one inside the new region, one outside
	v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	v->setVoxel(5, 5, 5, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	_sceneMgr->modified(nodeId, v->region());

	const voxel::Region oldRegion = v->region();
	EXPECT_EQ(oldRegion, (voxel::Region{0, 7}));

	// Shrink the volume - the voxel at (5,5,5) should be lost
	const voxel::Region newRegion{0, 3};
	_sceneMgr->nodeResize(nodeId, newRegion);

	v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	EXPECT_EQ(v->region(), newRegion) << "Volume should be resized to new region";
	EXPECT_TRUE(voxel::isBlocked(v->voxel(1, 1, 1).getMaterial())) << "Inner voxel should be preserved after shrink";

	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();

	for (int i = 0; i < 3; ++i) {
		// Undo the shrink - should restore both voxels
		ASSERT_TRUE(mementoHandler.canUndo());
		EXPECT_TRUE(_sceneMgr->undo());
		v = _sceneMgr->volume(nodeId);
		ASSERT_NE(nullptr, v);
		EXPECT_EQ(v->region(), oldRegion) << "Undo should restore original region (cycle " << i << ")";
		EXPECT_TRUE(voxel::isBlocked(v->voxel(1, 1, 1).getMaterial()))
			<< "Inner voxel should exist after undo (cycle " << i << ")";
		EXPECT_TRUE(voxel::isBlocked(v->voxel(5, 5, 5).getMaterial()))
			<< "Outer voxel should be restored after undo (cycle " << i << ")";

		// Redo the shrink
		ASSERT_TRUE(mementoHandler.canRedo());
		EXPECT_TRUE(_sceneMgr->redo());
		v = _sceneMgr->volume(nodeId);
		ASSERT_NE(nullptr, v);
		EXPECT_EQ(v->region(), newRegion) << "Redo should restore shrunk region (cycle " << i << ")";
		EXPECT_TRUE(voxel::isBlocked(v->voxel(1, 1, 1).getMaterial()))
			<< "Inner voxel should exist after redo (cycle " << i << ")";
	}
}

TEST_F(SceneManagerTest, testNodeQuantizeColors) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "testscene", voxel::Region{0, 3}));
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	scenegraph::SceneGraphNode &node = _sceneMgr->sceneGraph().node(nodeId);
	palette::Palette &palette = node.palette();

	// set up 4 similar red colors in the palette
	palette.setColor(0, color::RGBA(255, 0, 0, 255));   // pure red
	palette.setColor(1, color::RGBA(250, 5, 5, 255));   // near-red
	palette.setColor(2, color::RGBA(0, 0, 255, 255));   // pure blue
	palette.setColor(3, color::RGBA(5, 5, 250, 255));   // near-blue

	// place voxels with each color
	EXPECT_TRUE(testSetVoxel(glm::ivec3(0, 0, 0), 0));
	EXPECT_TRUE(testSetVoxel(glm::ivec3(1, 0, 0), 1));
	EXPECT_TRUE(testSetVoxel(glm::ivec3(2, 0, 0), 2));
	EXPECT_TRUE(testSetVoxel(glm::ivec3(3, 0, 0), 3));

	// quantize all 4 colors down to 2
	core::Buffer<uint8_t> selectedIndices;
	selectedIndices.push_back(0);
	selectedIndices.push_back(1);
	selectedIndices.push_back(2);
	selectedIndices.push_back(3);
	EXPECT_TRUE(_sceneMgr->nodeQuantizeColors(nodeId, selectedIndices, 2));

	// count how many palette slots are still active (alpha > 0)
	int activeColors = 0;
	for (int i = 0; i < 4; ++i) {
		if (palette.color(i).a > 0) {
			activeColors++;
		}
	}
	EXPECT_EQ(activeColors, 2) << "After quantizing 4 colors to 2, exactly 2 should remain active";

	// verify voxels are still present and use only active colors
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	for (int x = 0; x < 4; ++x) {
		const voxel::Voxel &voxel = v->voxel(x, 0, 0);
		EXPECT_TRUE(voxel::isBlocked(voxel.getMaterial())) << "Voxel at x=" << x << " should still exist";
		const uint8_t colorIdx = voxel.getColor();
		EXPECT_GT(palette.color(colorIdx).a, 0) << "Voxel at x=" << x << " should reference an active color";
	}
}

TEST_F(SceneManagerTest, testUndoInvalidatesTransformBrush) {
	// place a voxel, then activate the transform brush and capture a snapshot
	ASSERT_TRUE(testSetVoxel(testMins()));

	Modifier &modifier = _sceneMgr->modifier();
	modifier.endBrush();
	modifier.setBrushType(BrushType::Transform);
	TransformBrush &tb = modifier.transformBrush();

	// begin + preExecute to capture a snapshot
	EXPECT_TRUE(tb.beginBrush(modifier.brushContext()));
	tb.preExecute(modifier.brushContext(), testVolume());
	EXPECT_TRUE(tb.active());

	// undo the voxel placement - transform brush should be deactivated
	EXPECT_TRUE(_sceneMgr->undo());
	EXPECT_FALSE(tb.active());
}

TEST_F(SceneManagerTest, testUndoInvalidatesAABBBrush) {
	ASSERT_TRUE(testSetVoxel(testMins()));

	Modifier &modifier = _sceneMgr->modifier();
	modifier.endBrush();
	modifier.setBrushType(BrushType::Shape);
	ShapeBrush &sb = modifier.shapeBrush();

	// begin spanning an AABB
	modifier.setCursorPosition(testMins(), voxel::FaceNames::NegativeX);
	EXPECT_TRUE(sb.beginBrush(modifier.brushContext()));

	// undo - AABB mode should be cancelled
	EXPECT_TRUE(_sceneMgr->undo());
	EXPECT_FALSE(sb.aabbMode());
}

TEST_F(SceneManagerTest, testSplatMerge) {
	// Create a scene with two sibling model nodes that overlap at region [0,9]
	const voxel::Region region{0, 9};
	ASSERT_TRUE(_sceneMgr->newScene(true, "splatmerge_test", region));

	// Target node (first model node from newScene)
	const int targetNodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *targetVol = _sceneMgr->volume(targetNodeId);
	ASSERT_NE(nullptr, targetVol);
	targetVol->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	_sceneMgr->modified(targetNodeId, targetVol->region());

	// Create source as sibling (parent = root) so transforms are independent
	const int rootNodeId = _sceneMgr->sceneGraph().root().id();
	scenegraph::SceneGraphNode sourceNode(scenegraph::SceneGraphNodeType::Model);
	sourceNode.setVolume(new voxel::RawVolume(region), true);
	sourceNode.setName("source");
	const int sourceNodeId = _sceneMgr->moveNodeToSceneGraph(sourceNode, rootNodeId);
	ASSERT_NE(InvalidNodeId, sourceNodeId);
	voxel::RawVolume *sourceVol = _sceneMgr->volume(sourceNodeId);
	ASSERT_NE(nullptr, sourceVol);
	sourceVol->setVoxel(5, 5, 5, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	sourceVol->setVoxel(6, 6, 6, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	_sceneMgr->modified(sourceNodeId, sourceVol->region());

	ASSERT_TRUE(_sceneMgr->splatMerge(sourceNodeId));

	// Source node should be removed
	EXPECT_EQ(nullptr, _sceneMgr->sceneGraphNode(sourceNodeId));

	// Target should have the original voxel plus the merged ones
	EXPECT_FALSE(voxel::isAir(targetVol->voxel(0, 0, 0).getMaterial()));
	EXPECT_FALSE(voxel::isAir(targetVol->voxel(5, 5, 5).getMaterial()));
	EXPECT_FALSE(voxel::isAir(targetVol->voxel(6, 6, 6).getMaterial()));
	EXPECT_EQ(3, voxelutil::countVoxels(*targetVol));
}

TEST_F(SceneManagerTest, testSplatMergeNoOverlap) {
	const voxel::Region region{0, 4};
	ASSERT_TRUE(_sceneMgr->newScene(true, "splatmerge_nooverlap", region));

	const int targetNodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *targetVol = _sceneMgr->volume(targetNodeId);
	ASSERT_NE(nullptr, targetVol);
	targetVol->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	_sceneMgr->modified(targetNodeId, targetVol->region());

	// Create source at a non-overlapping region
	const voxel::Region farRegion{100, 104};
	const int rootNodeId = _sceneMgr->sceneGraph().root().id();
	scenegraph::SceneGraphNode sourceNode(scenegraph::SceneGraphNodeType::Model);
	sourceNode.setVolume(new voxel::RawVolume(farRegion), true);
	sourceNode.setName("source_far");
	const int sourceNodeId = _sceneMgr->moveNodeToSceneGraph(sourceNode, rootNodeId);
	ASSERT_NE(InvalidNodeId, sourceNodeId);
	voxel::RawVolume *sourceVol = _sceneMgr->volume(sourceNodeId);
	ASSERT_NE(nullptr, sourceVol);
	sourceVol->setVoxel(100, 100, 100, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	_sceneMgr->modified(sourceNodeId, sourceVol->region());

	// splatMerge should fail - no overlapping nodes
	EXPECT_FALSE(_sceneMgr->splatMerge(sourceNodeId));

	// Source node should still exist since merge failed
	EXPECT_NE(nullptr, _sceneMgr->sceneGraphNode(sourceNodeId));
}

TEST_F(SceneManagerTest, testGlobalCopyVisibleAndPasteNode) {
	const voxel::Region region{0, 4};
	ASSERT_TRUE(_sceneMgr->newScene(true, "globalcopyvisible_test", region));

	// Fill first node
	const int node1Id = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v1 = _sceneMgr->volume(node1Id);
	ASSERT_NE(nullptr, v1);
	v1->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	_sceneMgr->modified(node1Id, v1->region());

	// Create and fill second visible node
	const int node2Id = _sceneMgr->addModelChild("visible", 5, 5, 5);
	ASSERT_NE(-1, node2Id);
	ASSERT_TRUE(_sceneMgr->nodeActivate(node2Id));
	voxel::RawVolume *v2 = _sceneMgr->volume(node2Id);
	ASSERT_NE(nullptr, v2);
	v2->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	_sceneMgr->modified(node2Id, v2->region());

	// Global copy visible should succeed
	ASSERT_TRUE(_sceneMgr->globalCopyVisible());

	// Now paste as new node
	const size_t nodeCountBefore = _sceneMgr->sceneGraph().size();
	ASSERT_TRUE(_sceneMgr->globalPasteNode(glm::ivec3(0)));

	// A new node should have been created
	EXPECT_GT(_sceneMgr->sceneGraph().size(), nodeCountBefore);
}

TEST_F(SceneManagerTest, testAutoSelectOnPaste) {
	const voxel::Region region{0, 4};
	ASSERT_TRUE(_sceneMgr->newScene(true, "autoselect_paste", region));

	// Enable auto-select
	core::getVar(cfg::VoxEditAutoSelect)->setVal("true");

	// Set a voxel and select it for copy
	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	voxel::RawVolume *v = _sceneMgr->volume(nodeId);
	ASSERT_NE(nullptr, v);
	v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	v->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
	ASSERT_NE(nullptr, node);
	node->select(voxel::Region{glm::ivec3(0), glm::ivec3(1, 0, 0)});
	ASSERT_TRUE(node->hasSelection());

	// Copy and then paste
	ASSERT_TRUE(_sceneMgr->copy(nodeId));
	ASSERT_TRUE(_sceneMgr->paste(glm::ivec3(2, 0, 0)));

	// The pasted voxels should have FlagOutline set
	const voxel::Voxel &pasted1 = v->voxel(2, 0, 0);
	const voxel::Voxel &pasted2 = v->voxel(3, 0, 0);
	EXPECT_FALSE(voxel::isAir(pasted1.getMaterial()));
	EXPECT_FALSE(voxel::isAir(pasted2.getMaterial()));
	EXPECT_TRUE((pasted1.getFlags() & voxel::FlagOutline) != 0) << "Pasted voxel should be auto-selected";
	EXPECT_TRUE((pasted2.getFlags() & voxel::FlagOutline) != 0) << "Pasted voxel should be auto-selected";

	// The original voxels should NOT have FlagOutline (clearSelection was called)
	const voxel::Voxel &orig = v->voxel(0, 0, 0);
	EXPECT_TRUE((orig.getFlags() & voxel::FlagOutline) == 0) << "Original voxels should be deselected";

	// Disable auto-select for cleanup
	core::getVar(cfg::VoxEditAutoSelect)->setVal("false");
}

TEST_F(SceneManagerTest, testRedoInvalidatesBrushState) {
	ASSERT_TRUE(testSetVoxel(testMins()));
	EXPECT_TRUE(_sceneMgr->undo());

	Modifier &modifier = _sceneMgr->modifier();
	modifier.endBrush();
	modifier.setBrushType(BrushType::Transform);
	TransformBrush &tb = modifier.transformBrush();

	EXPECT_TRUE(tb.beginBrush(modifier.brushContext()));
	tb.preExecute(modifier.brushContext(), testVolume());
	EXPECT_TRUE(tb.active());

	// redo should also invalidate the transform brush state
	EXPECT_TRUE(_sceneMgr->redo());
	EXPECT_FALSE(tb.active());
}

} // namespace voxedit
