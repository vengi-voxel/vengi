/**
 * @file
 */

#include "../Viewport.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/PreviewManager.h"
#include "../WindowTitles.h"
#include "command/CommandHandler.h"
#include "app/Async.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "image/Image.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "util/VarUtil.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include "TestUtil.h"

namespace voxedit {

void Viewport::registerUITests(ImGuiTestEngine *engine, const char *) {
	IM_REGISTER_TEST(engine, testCategory(), "transform draft render")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		Viewport *viewport = viewportById(_app, viewportId);
		IM_CHECK(viewport != nullptr);
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNodeByUUID(_sceneMgr->activeNodeUUID());
		IM_CHECK(node != nullptr);
		voxel::RawVolume *volume = node->volume();
		const core::UUID uuid = node->uuid();
		const glm::ivec3 position = node->region().getCenter();
		const voxel::Region selection(position - glm::ivec3(2), position + glm::ivec3(2));
		voxel::Voxel selected = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		selected.setOutline();
		voxelutil::visitVolume(
			*volume, selection,
			[&](int x, int y, int z, const voxel::Voxel &) { volume->setVoxel(x, y, z, selected); },
			voxelutil::VisitAll());
		_sceneMgr->modified(uuid, selection);
		Modifier &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::Transform);
		TransformParameters parameters;
		parameters.translation = glm::ivec3(8, 0, 0);
		IM_CHECK(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
		ctx->Yield(5);
		IM_CHECK(viewport->_renderContext.sceneGraph == &modifier.renderSceneGraph());
		const scenegraph::SceneGraphNode *preview = viewport->_renderContext.sceneGraph->findNodeByUUID(uuid);
		IM_CHECK(preview != nullptr);
		const voxel::RawVolume *previewVolume = preview->volume();
		IM_CHECK(previewVolume != volume);
		IM_CHECK(voxel::isAir(previewVolume->voxel(position).getMaterial()));
		IM_CHECK(previewVolume->voxel(position + parameters.translation) == selected);
		IM_CHECK(volume->voxel(position) == selected);
		modifier.cancelDraft();
		ctx->Yield(5);
		IM_CHECK(viewport->_renderContext.sceneGraph == &_sceneMgr->sceneGraph());
		IM_CHECK(volume->voxel(position) == selected);
		IM_CHECK(modifier.updateParameters<TransformBrush>("transform", parameters).changed());
		IM_CHECK(modifier.applyDraft().changed());
		ctx->Yield(5);
		IM_CHECK(viewport->_renderContext.sceneGraph == &_sceneMgr->sceneGraph());
		IM_CHECK(voxel::isAir(volume->voxel(position).getMaterial()));
		IM_CHECK(volume->voxel(position + parameters.translation) == selected);
		IM_CHECK(_sceneMgr->undo());
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag shape keeps viewport input")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		if (isSceneMode()) {
			toggleScene();
		}
		IM_CHECK(focusWindow(ctx, Viewport::viewportId(_id).c_str()));
		ctx->Yield(2);
		resetCamera();
		util::ScopedVarChange brushGizmo(cfg::VoxEditBrushGizmo, "true");
		util::ScopedVarChange showAxis(cfg::VoxEditShowaxis, "true");
		util::ScopedVarChange showGrid(cfg::VoxEditShowgrid, "true");
		Modifier &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::Shape);
		ShapeBrush &brush = modifier.shapeBrush();
		brush.setBoxMode();
		brush.setRadius(0);
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, _id));
		ctx->Yield(2);
		command::executeCommands("+actionexecute 1 1");
		IM_CHECK(modifier.isEditActive());
		ctx->MouseDown(ImGuiMouseButton_Left);
		ctx->Yield(3);
		const bool capturedByGizmo = ImGuizmo::IsUsingAny();
		command::executeCommands("-actionexecute 1 1");
		ctx->MouseUp(ImGuiMouseButton_Left);
		ctx->Yield(3);
		IM_CHECK(!capturedByGizmo);
		IM_CHECK(!ImGuizmo::IsUsingAny());
		struct GridImages {
			image::ImagePtr withoutGrid;
			image::ImagePtr withGrid;
		};
		const core::SharedPtr<GridImages> images = core::make_shared<GridImages>();
		core::getVar(cfg::VoxEditShowgrid)->setVal(false);
		ctx->Yield(5);
		IM_CHECK(app::runOnMainThread(
			[this, images]() {
				video::FrameBuffer &display =
					_renderContext.enableMultisampling ? _renderContext.resolveFrameBuffer : _renderContext.frameBuffer;
				images->withoutGrid = display.image("without grid");
			},
			false));
		ctx->Yield(3);
		core::getVar(cfg::VoxEditShowgrid)->setVal(true);
		ctx->Yield(5);
		IM_CHECK(app::runOnMainThread(
			[this, images]() {
				video::FrameBuffer &display =
					_renderContext.enableMultisampling ? _renderContext.resolveFrameBuffer : _renderContext.frameBuffer;
				images->withGrid = display.image("with grid");
			},
			false));
		ctx->Yield(3);
		const image::ImagePtr &withoutGrid = images->withoutGrid;
		const image::ImagePtr &withGrid = images->withGrid;
		IM_CHECK(withoutGrid && withGrid);
		IM_CHECK(withoutGrid->size() == withGrid->size());
		const size_t bytes = (size_t)withGrid->width() * withGrid->height() * withGrid->components();
		IM_CHECK(core_memcmp(withoutGrid->data(), withGrid->data(), bytes) != 0);
		brush.setStrokeMode();
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, _id, ImVec2(40, 0)));
		ctx->Yield(3);
		IM_CHECK(!_sceneMgr->isViewportGizmoActive());
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNodeByUUID(_sceneMgr->activeNodeUUID());
		IM_CHECK(node != nullptr);
		voxel::RawVolume *volume = node->volume();
		const int before = voxelutil::countVoxels(*volume);
		executeViewportClick();
		IM_CHECK(voxelutil::countVoxels(*volume) > before);
		modifier.setBrushType(BrushType::Paint);
		PaintBrush &paint = modifier.paintBrush();
		paint.setStrokeMode();
		paint.setPaintMode(PaintBrush::PaintMode::Replace);
		modifier.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 3));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, _id, ImVec2(40, 0)));
		ctx->Yield(3);
		executeViewportClick();
		IM_CHECK(!ImGuizmo::IsUsingAny());
		int painted = 0;
		voxelutil::visitVolume(*volume, [&](int, int, int, const voxel::Voxel &value) {
			if (value.getColor() == 3) {
				++painted;
			}
		});
		IM_CHECK(painted > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "plane drag previews voxels until release")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		if (isSceneMode()) {
			toggleScene();
		}
		IM_CHECK(focusWindow(ctx, Viewport::viewportId(_id).c_str()));
		ctx->Yield(2);
		resetCamera();
		util::ScopedVarChange brushGizmo(cfg::VoxEditBrushGizmo, "true");
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNodeByUUID(_sceneMgr->activeNodeUUID());
		IM_CHECK(node != nullptr);
		voxel::RawVolume *volume = node->volume();
		const voxel::Region surface(0, 16, 0, 31, 16, 31);
		const voxel::Voxel ground(voxel::VoxelType::Generic, 1);
		voxelutil::visitVolume(*volume, surface,
			[&](int x, int y, int z, const voxel::Voxel &) { volume->setVoxel(x, y, z, ground); },
			voxelutil::VisitAll());
		_sceneMgr->modified(node->uuid(), surface);
		Modifier &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::Plane);
		modifier.setModifierType(ModifierType::Place);
		modifier.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 2));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, _id));
		ctx->Yield(5);
		IM_CHECK(modifier.cursorFace() == voxel::FaceNames::PositiveY);
		const int originalCount = voxelutil::countVoxels(*volume);
		const ImVec2 start = ImGui::GetIO().MousePos;
		command::executeCommands("+actionexecute 1 1");
		ctx->MouseDown(ImGuiMouseButton_Left);
		ctx->Yield(3);
		IM_CHECK(modifier.isEditActive());
		const voxel::RawVolume *preview = modifier.previewManager().previewVolume();
		IM_CHECK(preview != nullptr);
		IM_CHECK(preview->region().getHeightInVoxels() == 1);
		ctx->MouseMoveToPos(ImVec2(start.x, start.y - 60.0f));
		ctx->Yield(5);
		preview = modifier.previewManager().previewVolume();
		IM_CHECK(preview != nullptr);
		const voxel::Region filled = preview->region();
		const int height = filled.getHeightInVoxels();
		const int previewCount = voxelutil::countVoxels(*preview);
		core::DynamicArray<BrushGizmoState> handles;
		modifier.brushHandles(handles);
		const bool gizmoActive = ImGuizmo::IsUsingAny();
		const int heldCount = voxelutil::countVoxels(*volume);
		command::executeCommands("-actionexecute 1 1");
		ctx->MouseUp(ImGuiMouseButton_Left);
		ctx->Yield(3);
		IM_CHECK(height > 1);
		IM_CHECK(handles.empty());
		IM_CHECK(!gizmoActive);
		IM_CHECK(heldCount == originalCount);
		IM_CHECK(!modifier.isEditActive());
		IM_CHECK(voxelutil::countVoxels(*volume) == originalCount + previewCount);
	};

	IM_REGISTER_TEST(engine, testCategory(), "set voxel")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		resetCamera();
		ctx->Yield();
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId));
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNodeByUUID(_sceneMgr->activeNodeUUID());
		IM_CHECK(model != nullptr);
		const int cnt = voxelutil::countVoxels(*model->volume());
		IM_CHECK(cnt == 0);
		util::ScopedVarChange scoped(cfg::VoxEditGridsize, "1");
		ctx->Yield();
		executeViewportClick();
		IM_CHECK_EQ(1, voxelutil::countVoxels(*model->volume()));
	};

	IM_REGISTER_TEST(engine, testCategory(), "toggle rendering options")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
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
			const core::VarPtr &var = core::getVar(checkboxCvars[i]);
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
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(viewportId);

		// enable grid + bounding box + bones together
		const core::VarPtr &gridVar = core::getVar(cfg::VoxEditShowgrid);
		const core::VarPtr &aabbVar = core::getVar(cfg::VoxEditShowaabb);
		const core::VarPtr &bonesVar = core::getVar(cfg::VoxEditShowBones);
		const core::VarPtr &bloomVar = core::getVar(cfg::ClientBloom);
		const core::VarPtr &outlineVar = core::getVar(cfg::RenderOutline);
		const core::VarPtr &checkerVar = core::getVar(cfg::RenderCheckerBoard);
		const core::VarPtr &planeVar = core::getVar(cfg::VoxEditShowPlane);
		const core::VarPtr &gizmoVar = core::getVar(cfg::VoxEditShowaxis);

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
		IM_CHECK(prepareUITest(ctx));
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

	IM_REGISTER_TEST(engine, testCategory(), "recent colors select")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		ctx->Yield();

		// ensure this viewport is in edit mode
		if (isSceneMode()) {
			toggleScene();
		}
		IM_CHECK(!isSceneMode());

		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);
		// Recent-color buttons need enough menu-bar width after a dock layout reset
		// (camera combos and view toggles consume a large part of the bar).
		ctx->UndockWindow(vid.c_str());
		ctx->WindowResize(vid.c_str(), ImVec2(1600.0f, 700.0f));
		ctx->Yield(3);
		window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);
		ctx->WindowFocus(window->ID);
		ctx->Yield();
		ctx->ItemClick(window->ID);
		ctx->Yield();

		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNodeByUUID(_sceneMgr->activeNodeUUID());
		IM_CHECK(model != nullptr);

		Modifier &modifier = _sceneMgr->modifier();

		// clear any colors tracked during scene setup
		_recentColors.clear();

		const palette::Palette &palette = model->palette();
		// set two different cursor voxel colors to populate the MRU
		modifier.setCursorVoxel(voxel::createVoxel(palette, 1));
		ctx->Yield(3);
		IM_CHECK_EQ(_recentColors.size(), (size_t)1);

		modifier.setCursorVoxel(voxel::createVoxel(palette, 5));
		ctx->Yield(3);
		IM_CHECK_EQ(_recentColors.size(), (size_t)2);
		IM_CHECK_EQ(_recentColors[0], palette.color(5));
		IM_CHECK_EQ(_recentColors[1], palette.color(1));

		// click the second recent color (index 1) to select it
		ctx->SetRef(window);
		IM_CHECK(ctx->ItemExists("##MenuBar/##recentcol1"));
		ctx->ItemClick("##MenuBar/##recentcol1");
		ctx->Yield(3);

		// the cursor voxel should now be color index 1
		IM_CHECK_EQ(modifier.cursorVoxel().getColor(), (uint8_t)1);
		// and the MRU should have reordered: 1 is now most recent
		IM_CHECK_EQ(_recentColors[0], palette.color(1));
		IM_CHECK_EQ(_recentColors[1], palette.color(5));
	};

	IM_REGISTER_TEST(engine, testCategory(), "scene mode with nodes")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		ctx->Yield();

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int firstNode = sceneGraph.activeNode();

		// add a second model node
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.setName("second node");
		newNode.createVolume(voxel::Region(0, 31));
		const int secondNode = _sceneMgr->moveNodeToSceneGraph(newNode, sceneGraph.root().id());
		IM_CHECK(secondNode != InvalidNodeId);
		IM_CHECK_EQ(sceneGraph.size(scenegraph::SceneGraphNodeType::Model), 2u);

		// switch to scene mode
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		ctx->Yield();

		// activate each node
		_sceneMgr->nodeActivate(_sceneMgr->sceneGraph().uuid(firstNode));
		ctx->Yield();
		IM_CHECK_EQ(sceneGraph.activeNode(), firstNode);

		_sceneMgr->nodeActivate(_sceneMgr->sceneGraph().uuid(secondNode));
		ctx->Yield();
		IM_CHECK_EQ(sceneGraph.activeNode(), secondNode);
	};

	IM_REGISTER_TEST(engine, testCategory(), "camera projection combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);
		ctx->WindowFocus(window->ID);
		ctx->Yield();
		ctx->SetRef(window);

		// verify the camera projection combo exists in the menu bar
		const ImGuiTestItemInfo projInfo = ctx->ItemInfo("##MenuBar/###cameraproj", ImGuiTestOpFlags_NoError);
		IM_CHECK(projInfo.ID != 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "camera mode combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);
		ctx->WindowFocus(window->ID);
		ctx->Yield();
		ctx->SetRef(window);

		// the camera mode combo should exist in the menu bar
		const ImGuiTestItemInfo info = ctx->ItemInfo("##MenuBar/###cameramode", ImGuiTestOpFlags_NoError);
		IM_CHECK(info.ID != 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "slicer toggle")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int viewportId = prepareBrushViewport(ctx, _sceneMgr, _app, "viewportslicer");
		IM_CHECK(viewportId != -1);

		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);
		ctx->WindowFocus(window->ID);
		ctx->Yield();
		ctx->SetRef(window);

		// the slicer checkbox should exist
		const ImGuiTestItemInfo slicerInfo = ctx->ItemInfo("##sliceactive", ImGuiTestOpFlags_NoError);
		IM_CHECK(slicerInfo.ID != 0);

		// toggle slicer on
		IM_CHECK(!_sceneMgr->isSliceModeActive());
		ctx->ItemClick("##sliceactive");
		ctx->Yield();
		IM_CHECK(_sceneMgr->isSliceModeActive());

		// toggle slicer off
		ctx->ItemClick("##sliceactive");
		ctx->Yield();
		IM_CHECK(!_sceneMgr->isSliceModeActive());
	};

	IM_REGISTER_TEST(engine, testCategory(), "screenshot menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);
		ctx->WindowFocus(window->ID);
		ctx->Yield();
		ctx->SetRef(window);

		// the screenshot menu item should exist under View
		ctx->MenuClick("View/Screenshot");
		ctx->Yield();
		// screenshot triggers a save dialog - cancel it
		cancelSaveFile(ctx);
	};

	IM_REGISTER_TEST(engine, testCategory(), "video recording toggle")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		ctx->Yield();

		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(window != nullptr);
		ctx->WindowFocus(window->ID);
		ctx->Yield();
		ctx->SetRef(window);

		// click the Video menu item to start recording
		ctx->MenuClick("View/Video");
		ctx->Yield();
		// if recording started, it opens a save dialog - cancel it
		cancelSaveFile(ctx);
	};

	IM_REGISTER_TEST(engine, testCategory(), "camera node frame change")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		ctx->Yield(2);

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// add a camera node
		scenegraph::SceneGraphNodeCamera camNode;
		camNode.setName("testcam");
		camNode.setPerspective();
		camNode.setFieldOfView(60);
		camNode.setNearPlane(0.1f);
		camNode.setFarPlane(500.0f);

		// set transform at frame 0 with a known position
		scenegraph::SceneGraphTransform transform0;
		transform0.setWorldTranslation(glm::vec3(0.0f, 0.0f, 0.0f));
		transform0.setWorldOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		transform0.setWorldScale(glm::vec3(1.0f));
		camNode.setTransform(0, transform0);

		const int camNodeId = _sceneMgr->moveNodeToSceneGraph(camNode, sceneGraph.root().id());
		IM_CHECK(camNodeId != InvalidNodeId);
		ctx->Yield(2);

		// add a keyframe at frame 10 with a different position
		scenegraph::SceneGraphNode &camRef = sceneGraph.node(camNodeId);
		const scenegraph::KeyFrameIndex kfIdx = camRef.addKeyFrame(10);
		IM_CHECK(kfIdx != InvalidKeyFrame);
		scenegraph::SceneGraphTransform transform10;
		transform10.setWorldTranslation(glm::vec3(100.0f, 0.0f, 0.0f));
		transform10.setWorldOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		transform10.setWorldScale(glm::vec3(1.0f));
		camRef.setTransform(kfIdx, transform10);

		// activate the camera node so activeCameraNode() returns it
		_sceneMgr->nodeActivate(_sceneMgr->sceneGraph().uuid(camNodeId));
		ctx->Yield(2);
		IM_CHECK(sceneGraph.activeNode() == camNodeId);
		IM_CHECK(_sceneMgr->activeCameraNode() != nullptr);

		// set frame to 0 and let the viewport update
		_sceneMgr->setCurrentFrame(0);
		ctx->Yield(3);
		const glm::vec3 posAtFrame0 = _camera.worldPosition();

		// change to frame 10 (manual scrub)
		_sceneMgr->setCurrentFrame(10);
		ctx->Yield(3);
		const glm::vec3 posAtFrame10 = _camera.worldPosition();

		// the camera should have moved
		IM_CHECK(glm::distance(posAtFrame0, posAtFrame10) > 1.0f);
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag palette color to viewport")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int viewportId = prepareBrushViewport(ctx, _sceneMgr, _app, "viewportdragcolor");
		IM_CHECK(viewportId != -1);

		// find the palette panel window to get a cell position
		ImGuiWindow *palWindow = ImGui::FindWindowByName(TITLE_PALETTE);
		if (palWindow == nullptr || !palWindow->Active) {
			return; // palette panel not visible in this layout
		}

		// get the viewport window center
		const core::String vid = Viewport::viewportId(_id);
		ImGuiWindow *vpWindow = ImGui::FindWindowByName(vid.c_str());
		IM_CHECK(vpWindow != nullptr);
		const ImVec2 vpCenter = vpWindow->Rect().GetCenter();

		// get palette cell 0 position
		const float frameHeight = ImGui::GetFrameHeight();
		const ImVec2 palPos(palWindow->ContentRegionRect.Min.x + frameHeight * 0.5f,
							palWindow->ContentRegionRect.Min.y + frameHeight * 0.5f);

		// drag from palette cell to viewport center using the proper drag pattern
		dragFromTo(ctx, palPos, vpCenter);
	};
}

} // namespace voxedit
