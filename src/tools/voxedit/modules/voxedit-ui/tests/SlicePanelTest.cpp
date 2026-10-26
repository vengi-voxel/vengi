/**
 * @file
 */

#include "../SlicePanel.h"
#include "TestUtil.h"
#include "util/VarUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void SlicePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "edit all planes and undo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		util::ScopedVarChange visible(cfg::VoxEditShowSlices, "false");
		IM_CHECK(ensurePanelVisible(ctx, "View/2D Views", cfg::VoxEditShowSlices));
		const voxel::Region region(glm::ivec3(-3, -4, -5), glm::ivec3(4, 3, 2));
		IM_CHECK(_sceneMgr->newScene(true, "slices", region));
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 42));
		const char *canvases[] = {"**/backcanvas", "**/rightcanvas", "**/topcanvas"};
		for (int view = 0; view < 3; ++view) {
			const ImGuiTestItemInfo info = ctx->ItemInfo(canvases[view]);
			IM_CHECK(info.ID != 0);
			const ImVec2 min = info.RectFull.Min;
			const ImVec2 max = info.RectFull.Max;
			const float cell = core_min((max.x - min.x) / 8.0f, (max.y - min.y) / 8.0f);
			const ImVec2 origin((min.x + max.x - 8.0f * cell) * 0.5f, (min.y + max.y - 8.0f * cell) * 0.5f);
			const glm::ivec3 pos = voxelPosition(view, region, _slices[view], 2, 3);
			ctx->MouseMoveToPos(ImVec2(origin.x + 2.5f * cell, origin.y + 3.5f * cell));
			ctx->MouseClick(ImGuiMouseButton_Left);
			ctx->Yield();
			voxel::RawVolume *volume = _sceneMgr->volume(_sceneMgr->activeNodeUUID());
			IM_CHECK_EQ(volume->voxel(pos).getColor(), 42);
			IM_CHECK(!voxel::isAir(volume->voxel(pos).getMaterial()));
			IM_CHECK(_sceneMgr->undo());
			IM_CHECK(voxel::isAir(volume->voxel(pos).getMaterial()));
			IM_CHECK(_sceneMgr->redo());
			IM_CHECK_EQ(volume->voxel(pos).getColor(), 42);
			ctx->MouseClick(ImGuiMouseButton_Right);
			ctx->Yield();
			IM_CHECK(voxel::isAir(volume->voxel(pos).getMaterial()));
			IM_CHECK(_sceneMgr->undo());
			IM_CHECK_EQ(volume->voxel(pos).getColor(), 42);
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag stroke only edits selected model")->TestFunc =
		[=](ImGuiTestContext *ctx) {
			IM_CHECK(prepareUITest(ctx));
			util::ScopedVarChange visible(cfg::VoxEditShowSlices, "true");
			const voxel::Region region(0, 7);
			IM_CHECK(_sceneMgr->newScene(true, "stroke", region));
			const int active = _sceneMgr->activeNode();
			scenegraph::SceneGraphNode other(scenegraph::SceneGraphNodeType::Model);
			other.createVolume(region);
			const int otherId = _sceneMgr->moveNodeToSceneGraph(other);
			IM_CHECK(otherId != InvalidNodeId);
			IM_CHECK(_sceneMgr->nodeActivate(_sceneMgr->sceneGraph().node(active).uuid()));
			ctx->Yield();
			IM_CHECK(focusWindow(ctx, id));
			_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 42));
			const ImGuiTestItemInfo info = ctx->ItemInfo("**/backcanvas");
			IM_CHECK(info.ID != 0);
			const ImVec2 min = info.RectFull.Min;
			const ImVec2 max = info.RectFull.Max;
			const float cell = core_min((max.x - min.x) / 8.0f, (max.y - min.y) / 8.0f);
			const ImVec2 origin((min.x + max.x - 8.0f * cell) * 0.5f, (min.y + max.y - 8.0f * cell) * 0.5f);
			ctx->MouseMoveToPos(ImVec2(origin.x + 1.5f * cell, origin.y + 2.5f * cell));
			ctx->MouseDown(ImGuiMouseButton_Left);
			ctx->MouseMoveToPos(ImVec2(origin.x + 6.5f * cell, origin.y + 2.5f * cell));
			ctx->MouseUp(ImGuiMouseButton_Left);
			ctx->Yield();
			IM_CHECK_EQ(voxelutil::countVoxels(*_sceneMgr->sceneGraph().node(active).volume()), 6);
			IM_CHECK_EQ(voxelutil::countVoxels(*_sceneMgr->sceneGraph().node(otherId).volume()), 0);
			IM_CHECK(_sceneMgr->undo());
			IM_CHECK_EQ(voxelutil::countVoxels(*_sceneMgr->sceneGraph().node(active).volume()), 0);
			IM_CHECK(_sceneMgr->redo());
			IM_CHECK_EQ(voxelutil::countVoxels(*_sceneMgr->sceneGraph().node(active).volume()), 6);
			_background = true;
			ctx->Yield();
			_background = false;
		};

	IM_REGISTER_TEST(engine, testCategory(), "coordinates and locked model")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		const voxel::Region region(glm::ivec3(-3, -4, -5), glm::ivec3(4, 3, 2));
		IM_CHECK(voxelPosition(0, region, -2, 1, 2) == glm::ivec3(-2, 1, -2));
		IM_CHECK(voxelPosition(1, region, -1, 1, 2) == glm::ivec3(-1, 1, 1));
		IM_CHECK(voxelPosition(2, region, 0, 1, 2) == glm::ivec3(-2, 0, -3));
		IM_CHECK(_sceneMgr->newScene(true, "locked slices", region));
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
		IM_CHECK(node != nullptr);
		node->setLocked(true);
		const glm::ivec3 pos = region.getLowerCorner();
		_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 42));
		editLine(*node, pos, pos, false);
		IM_CHECK(voxel::isAir(node->volume()->voxel(pos).getMaterial()));
		node->setLocked(false);
		const voxel::Voxel selected = voxel::createVoxel(voxel::VoxelType::Generic, 7, NO_NORMAL, voxel::FlagOutline);
		IM_CHECK(node->volume()->setVoxel(pos, selected));
		const glm::ivec3 otherPos = pos + glm::ivec3(1, 0, 0);
		IM_CHECK(node->volume()->setVoxel(otherPos, voxel::createVoxel(voxel::VoxelType::Generic, 8)));
		_sceneMgr->modified(node->uuid(), region);
		editLine(*node, pos, otherPos, false);
		IM_CHECK_EQ(node->volume()->voxel(pos).getColor(), 42);
		IM_CHECK_EQ(node->volume()->voxel(pos).getFlags(), voxel::FlagOutline);
		IM_CHECK_EQ(node->volume()->voxel(otherPos).getColor(), 8);
		const size_t states = _sceneMgr->mementoHandler().stateSize();
		editLine(*node, pos, pos, false);
		IM_CHECK_EQ(_sceneMgr->mementoHandler().stateSize(), states);
	};
}

} // namespace voxedit
