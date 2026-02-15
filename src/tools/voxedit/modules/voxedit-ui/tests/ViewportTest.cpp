/**
 * @file
 */

#include "../Viewport.h"
#include "command/CommandHandler.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include "TestUtil.h"

namespace voxedit {

void Viewport::registerUITests(ImGuiTestEngine *engine, const char *) {
	IM_REGISTER_TEST(engine, testCategory(), "set voxel")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		const int cnt = voxelutil::countVoxels(*model->volume());
		IM_CHECK(cnt == 0);
		util::ScopedVarChange scoped(cfg::VoxEditGridsize, "1");
		ctx->Yield();
		executeViewportClick();
		IM_CHECK_EQ(1, voxelutil::countVoxels(*model->volume()));
	};

	IM_REGISTER_TEST(engine, testCategory(), "toggle rendering options")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(viewportId);

		// toggle each rendering option on and off individually via the View menu
		const char *checkboxItems[] = {"Grid", "Show gizmo", "Show locked axis", "Bounding box",
									   "Bones", "Plane", "Outlines", "Checkerboard", "Bloom"};
		const char *checkboxCvars[] = {cfg::VoxEditShowgrid, cfg::VoxEditShowaxis, cfg::VoxEditShowlockedaxis,
									   cfg::VoxEditShowaabb, cfg::VoxEditShowBones, cfg::VoxEditShowPlane,
									   cfg::RenderOutline, cfg::RenderCheckerBoard, cfg::ClientBloom};

		for (int i = 0; i < (int)lengthof(checkboxItems); ++i) {
			const core::VarPtr &var = core::Var::getSafe(checkboxCvars[i]);
			const bool initialVal = var->boolVal();

			// toggle on
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick(core::String::format("View/%s", checkboxItems[i]).c_str());
			ctx->Yield(3);
			IM_CHECK_EQ(var->boolVal(), !initialVal);

			// toggle back
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick(core::String::format("View/%s", checkboxItems[i]).c_str());
			ctx->Yield(3);
			IM_CHECK_EQ(var->boolVal(), initialVal);
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "toggle rendering combinations")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(viewportId);

		// enable grid + bounding box + bones together
		const core::VarPtr &gridVar = core::Var::getSafe(cfg::VoxEditShowgrid);
		const core::VarPtr &aabbVar = core::Var::getSafe(cfg::VoxEditShowaabb);
		const core::VarPtr &bonesVar = core::Var::getSafe(cfg::VoxEditShowBones);
		const core::VarPtr &bloomVar = core::Var::getSafe(cfg::ClientBloom);
		const core::VarPtr &outlineVar = core::Var::getSafe(cfg::RenderOutline);
		const core::VarPtr &checkerVar = core::Var::getSafe(cfg::RenderCheckerBoard);
		const core::VarPtr &planeVar = core::Var::getSafe(cfg::VoxEditShowPlane);
		const core::VarPtr &gizmoVar = core::Var::getSafe(cfg::VoxEditShowaxis);

		// save initial states to restore later
		const bool gridInit = gridVar->boolVal();
		const bool aabbInit = aabbVar->boolVal();
		const bool bonesInit = bonesVar->boolVal();
		const bool bloomInit = bloomVar->boolVal();
		const bool outlineInit = outlineVar->boolVal();
		const bool checkerInit = checkerVar->boolVal();
		const bool planeInit = planeVar->boolVal();
		const bool gizmoInit = gizmoVar->boolVal();

		// combination 1: enable grid + bounding box + bones
		if (!gridInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Grid");
			ctx->Yield();
		}
		if (!aabbInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Bounding box");
			ctx->Yield();
		}
		if (!bonesInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Bones");
			ctx->Yield();
		}
		IM_CHECK(gridVar->boolVal());
		IM_CHECK(aabbVar->boolVal());
		IM_CHECK(bonesVar->boolVal());
		ctx->Yield(3);

		// combination 2: add bloom + outlines + checkerboard on top
		if (!bloomInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Bloom");
			ctx->Yield();
		}
		if (!outlineInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Outlines");
			ctx->Yield();
		}
		if (!checkerInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Checkerboard");
			ctx->Yield();
		}
		IM_CHECK(bloomVar->boolVal());
		IM_CHECK(outlineVar->boolVal());
		IM_CHECK(checkerVar->boolVal());
		ctx->Yield(3);

		// combination 3: disable some while keeping others - toggle grid and bloom off
		IM_CHECK(focusWindow(ctx, vid.c_str()));
		ctx->MenuClick("View/Grid");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, vid.c_str()));
		ctx->MenuClick("View/Bloom");
		ctx->Yield(3);
		IM_CHECK(!gridVar->boolVal());
		IM_CHECK(!bloomVar->boolVal());
		// others should still be enabled
		IM_CHECK(aabbVar->boolVal());
		IM_CHECK(bonesVar->boolVal());
		IM_CHECK(outlineVar->boolVal());
		IM_CHECK(checkerVar->boolVal());

		// combination 4: enable plane + gizmo with the remaining options
		if (!planeInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Plane");
			ctx->Yield();
		}
		if (!gizmoInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Show gizmo");
			ctx->Yield();
		}
		IM_CHECK(planeVar->boolVal());
		IM_CHECK(gizmoVar->boolVal());
		ctx->Yield(3);

		// restore all to initial state
		if (gridVar->boolVal() != gridInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Grid");
			ctx->Yield();
		}
		if (aabbVar->boolVal() != aabbInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Bounding box");
			ctx->Yield();
		}
		if (bonesVar->boolVal() != bonesInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Bones");
			ctx->Yield();
		}
		if (bloomVar->boolVal() != bloomInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Bloom");
			ctx->Yield();
		}
		if (outlineVar->boolVal() != outlineInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Outlines");
			ctx->Yield();
		}
		if (checkerVar->boolVal() != checkerInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Checkerboard");
			ctx->Yield();
		}
		if (planeVar->boolVal() != planeInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Plane");
			ctx->Yield();
		}
		if (gizmoVar->boolVal() != gizmoInit) {
			IM_CHECK(focusWindow(ctx, vid.c_str()));
			ctx->MenuClick("View/Show gizmo");
			ctx->Yield();
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "toggle scene mode")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);

		// ensure we start in edit mode
		ctx->WindowFocus(window->ID);
		ctx->Yield();
		ctx->ItemClick(window->ID);
		ctx->Yield();
		ctx->SetRef(window);
		if (isSceneMode()) {
			ctx->ItemClick("##MenuBar/Scene Mode");
			ctx->Yield(3);
		}
		IM_CHECK(!isSceneMode());

		// switch to scene mode
		ctx->ItemClick("##MenuBar/Scene Mode");
		ctx->Yield(3);
		IM_CHECK(isSceneMode());

		// switch back to edit mode
		ctx->ItemClick("##MenuBar/Scene Mode");
		ctx->Yield(3);
		IM_CHECK(!isSceneMode());
	};

	IM_REGISTER_TEST(engine, testCategory(), "scene mode with nodes")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "scenenodetest", voxel::Region(0, 31)));
		ctx->Yield();

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int firstNode = sceneGraph.activeNode();

		// add a second model node
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.setName("second node");
		newNode.setVolume(new voxel::RawVolume(voxel::Region(0, 31)), true);
		const int secondNode = _sceneMgr->moveNodeToSceneGraph(newNode, sceneGraph.root().id());
		IM_CHECK(secondNode != InvalidNodeId);
		IM_CHECK_EQ(sceneGraph.size(scenegraph::SceneGraphNodeType::Model), 2);

		// switch to scene mode
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		ctx->Yield();

		// activate each node
		_sceneMgr->nodeActivate(firstNode);
		ctx->Yield();
		IM_CHECK_EQ(sceneGraph.activeNode(), firstNode);

		_sceneMgr->nodeActivate(secondNode);
		ctx->Yield();
		IM_CHECK_EQ(sceneGraph.activeNode(), secondNode);
	};
}

} // namespace voxedit
