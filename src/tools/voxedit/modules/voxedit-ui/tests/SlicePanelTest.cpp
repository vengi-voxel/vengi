/**
 * @file
 */

#include "../SlicePanel.h"
#include "TestUtil.h"
#include "command/CommandHandler.h"
#include "core/BindingContext.h"
#include "core/ConfigVar.h"
#include "ui/IMGUIStyle.h"
#include "ui/Style.h"
#include "util/VarUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include <SDL3/SDL.h>

namespace voxedit {

void SlicePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "model bindings pick slice color")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		util::ScopedVarChange visible(cfg::VoxEditShowSlices, "true");
		IM_CHECK(_sceneMgr->newScene(true, "slice bindings", voxel::Region(0, 7)));
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		const char *canvases[] = {"**/backcanvas", "**/rightcanvas", "**/topcanvas"};
		for (int view = 0; view < 3; ++view) {
			ctx->MouseMove(canvases[view]);
			ctx->Yield();
			const TargetView &targetView = _targetViews[view];
			const glm::ivec3 pos = voxelPosition(view, voxel::Region(0, 7), _slices[view], 2, 3);
			scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
			IM_CHECK(setVoxel(_sceneMgr, node, pos, voxel::createVoxel(voxel::VoxelType::Generic, 42)));
			ctx->MouseMoveToPos(
				ImVec2(targetView.origin.x + 2.5f * targetView.cell, targetView.origin.y + 3.5f * targetView.cell));
			ctx->Yield(2);
			IM_CHECK(_sceneMgr->modifier().isSliceMode());
			IM_CHECK(isHovered());
			_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 7));
			SDL_Event event{};
			event.type = SDL_EVENT_KEY_DOWN;
			event.key.windowID = SDL_GetWindowID((SDL_Window *)_app->windowHandle());
			event.key.key = SDLK_C;
			event.key.scancode = SDL_SCANCODE_C;
			SDL_PushEvent(&event);
			ctx->Yield(2);
			event.type = SDL_EVENT_KEY_UP;
			SDL_PushEvent(&event);
			ctx->Yield();
			IM_CHECK_EQ(_sceneMgr->modifier().cursorVoxel().getColor(), 42);
		}
		SDL_CleanupTLS();
	};

	IM_REGISTER_TEST(engine, testCategory(), "select and translate on each plane")->TestFunc =
		[=](ImGuiTestContext *ctx) {
			IM_CHECK(prepareUITest(ctx));
			util::ScopedVarChange visible(cfg::VoxEditShowSlices, "true");
			const voxel::Region region(0, 7);
			const char *canvases[] = {"**/backcanvas", "**/rightcanvas", "**/topcanvas"};
			const int depthAxes[] = {2, 0, 1};
			for (int view = 0; view < 3; ++view) {
				IM_CHECK(_sceneMgr->newScene(true, "slice selection", region));
				ctx->Yield();
				IM_CHECK(focusWindow(ctx, id));
				command::executeCommands("brushselect");
				_sceneMgr->modifier().selectBrush().setSelectMode(SelectMode::Box3D);
				ctx->MouseMove(canvases[view]);
				ctx->Yield();
				const TargetView targetView = _targetViews[view];
				scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
				const glm::ivec3 first = voxelPosition(view, region, _slices[view], 2, 3);
				const glm::ivec3 second = voxelPosition(view, region, _slices[view], 3, 3);
				glm::ivec3 otherSlice = first;
				++otherSlice[depthAxes[view]];
				const voxel::Voxel value = voxel::createVoxel(voxel::VoxelType::Generic, 42);
				IM_CHECK(setVoxel(_sceneMgr, node, first, value));
				IM_CHECK(setVoxel(_sceneMgr, node, second, value));
				IM_CHECK(setVoxel(_sceneMgr, node, otherSlice, value));
				ctx->MouseMoveToPos(
					ImVec2(targetView.origin.x + 2.5f * targetView.cell, targetView.origin.y + 3.5f * targetView.cell));
				command::executeCommands("+actionexecute 1 1");
				ctx->Yield();
				ctx->MouseMoveToPos(
					ImVec2(targetView.origin.x + 3.5f * targetView.cell, targetView.origin.y + 3.5f * targetView.cell));
				command::executeCommands("-actionexecute 1 1");
				ctx->Yield();
				IM_CHECK(node->volume()->voxel(first).getFlags() & voxel::FlagOutline);
				IM_CHECK(node->volume()->voxel(second).getFlags() & voxel::FlagOutline);
				IM_CHECK_EQ(node->volume()->voxel(otherSlice).getFlags(), 0);
				const voxel::Region selected = sliceSelection(*node, view);
				const glm::ivec2 uv = viewPosition(view, region, selected.getCenter());
				const ImVec2 handle(targetView.origin.x + (uv.x + 0.5f) * targetView.cell + ImGui::Size(3.5f),
									targetView.origin.y + (uv.y + 0.5f) * targetView.cell);
				ctx->MouseMoveToPos(handle);
				ctx->Yield();
				IM_CHECK(_gizmoHovered);
				const size_t beforeMove = _sceneMgr->mementoHandler().stateSize();
				command::executeCommands("+actionexecute 1 1");
				ctx->Yield();
				IM_CHECK_EQ(_gizmoView, view);
				const math::Axis horizontalAxes[] = {math::Axis::X, math::Axis::Z, math::Axis::X};
				IM_CHECK_EQ(_gizmoAxis, horizontalAxes[view]);
				ctx->MouseMoveToPos(ImVec2(handle.x + 2.0f * targetView.cell, handle.y));
				ctx->Yield();
				const glm::ivec3 offset =
					voxelPosition(view, region, _slices[view], 2, 0) - voxelPosition(view, region, _slices[view], 0, 0);
				IM_CHECK(_gizmoPreview != nullptr);
				IM_CHECK_EQ(node->volume()->voxel(first).getColor(), 42);
				IM_CHECK(voxel::isAir(node->volume()->voxel(first + offset).getMaterial()));
				IM_CHECK(voxel::isAir(displayVoxel(*node->volume(), first).getMaterial()));
				IM_CHECK_EQ(displayVoxel(*node->volume(), first + offset).getColor(), 42);
				IM_CHECK_EQ(displayVoxel(*node->volume(), second + offset).getColor(), 42);
				IM_CHECK_EQ(_sceneMgr->mementoHandler().stateSize(), beforeMove);
				for (const TargetView &drawnView : _targetViews) {
					bool targetGuide = false;
					for (const ImDrawVert &vertex : drawnView.draw->VtxBuffer) {
						if (vertex.col == ImGui::GetColorU32(ImVec4(style::color(style::ColorBrushGizmoLine)))) {
							targetGuide |=
								vertex.pos.x <= drawnView.min.x + 1.0f || vertex.pos.x >= drawnView.max.x - 1.0f ||
								vertex.pos.y <= drawnView.min.y + 1.0f || vertex.pos.y >= drawnView.max.y - 1.0f;
						}
					}
					IM_CHECK(!targetGuide);
				}
				const glm::ivec2 destination = viewPosition(view, region, first + offset);
				const color::RGBA color = node->palette().color(42);
				bool previewDrawn = false;
				for (const ImDrawVert &vertex : _targetViews[view].draw->VtxBuffer) {
					previewDrawn |=
						vertex.col == IM_COL32(color.r, color.g, color.b, 255) &&
						glm::abs(vertex.pos.x - (targetView.origin.x + destination.x * targetView.cell)) < 1.0f &&
						glm::abs(vertex.pos.y - (targetView.origin.y + destination.y * targetView.cell)) < 1.0f;
				}
				IM_CHECK(previewDrawn);
				command::executeCommands("-actionexecute 1 1");
				ctx->Yield();
				IM_CHECK(_gizmoPreview == nullptr);
				IM_CHECK_EQ(_sceneMgr->mementoHandler().stateSize(), beforeMove + 1);
				IM_CHECK(voxel::isAir(node->volume()->voxel(first).getMaterial()));
				IM_CHECK_EQ(node->volume()->voxel(first + offset).getColor(), 42);
				IM_CHECK_EQ(node->volume()->voxel(second + offset).getColor(), 42);
				IM_CHECK_EQ(node->volume()->voxel(otherSlice).getColor(), 42);
				IM_CHECK(_sceneMgr->undo());
				IM_CHECK_EQ(node->volume()->voxel(first).getColor(), 42);
				IM_CHECK(voxel::isAir(node->volume()->voxel(second + offset).getMaterial()));
				IM_CHECK(_sceneMgr->redo());
				IM_CHECK_EQ(node->volume()->voxel(second + offset).getColor(), 42);
				ctx->MouseMoveToPos(
					ImVec2(targetView.origin.x + 0.5f * targetView.cell, targetView.origin.y + 0.5f * targetView.cell));
				ctx->Yield();
				IM_CHECK(!_gizmoHovered);
				IM_CHECK_EQ(_gizmoView, -1);
				ctx->Yield(3);
			}
			_sceneMgr->modifier().setBrushType(BrushType::Shape);
		};

	IM_REGISTER_TEST(engine, testCategory(), "brush options operate on a single slice")->TestFunc =
		[=](ImGuiTestContext *ctx) {
			IM_CHECK(prepareUITest(ctx));
			util::ScopedVarChange visible(cfg::VoxEditShowSlices, "true");
			const voxel::Region region(0, 7);
			const char *canvases[] = {"**/backcanvas", "**/rightcanvas", "**/topcanvas"};
			const int depthAxes[] = {2, 0, 1};
			for (int view = 0; view < 3; ++view) {
				IM_CHECK(_sceneMgr->newScene(true, "shared slice brush", region));
				ctx->Yield();
				IM_CHECK(focusWindow(ctx, id));
				ctx->MouseMove(canvases[view]);
				ctx->Yield();
				const TargetView targetView = _targetViews[view];
				ctx->MouseMoveToPos(
					ImVec2(targetView.origin.x + 3.5f * targetView.cell, targetView.origin.y + 3.5f * targetView.cell));
				Modifier &modifier = _sceneMgr->modifier();
				modifier.setBrushType(BrushType::Shape);
				modifier.shapeBrush().setRadius(2);
				modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 7));
				command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
				ctx->Yield();
				scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
				IM_CHECK(voxelutil::countVoxels(*node->volume()) > 1);
				for (int z = 0; z < 8; ++z) {
					for (int y = 0; y < 8; ++y) {
						for (int x = 0; x < 8; ++x) {
							const glm::ivec3 pos(x, y, z);
							if (!voxel::isAir(node->volume()->voxel(pos).getMaterial())) {
								IM_CHECK_EQ(pos[depthAxes[view]], _slices[view]);
							}
						}
					}
				}
				modifier.setBrushType(BrushType::Paint);
				modifier.paintBrush().setRadius(2);
				modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 42));
				command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
				ctx->Yield();
				const glm::ivec3 pos = voxelPosition(view, region, _slices[view], 3, 3);
				IM_CHECK_EQ(node->volume()->voxel(pos).getColor(), 42);
				glm::ivec3 otherSlice = pos;
				++otherSlice[depthAxes[view]];
				IM_CHECK(setVoxel(_sceneMgr, node, otherSlice, voxel::createVoxel(voxel::VoxelType::Generic, 42)));
				modifier.setBrushType(BrushType::Select);
				modifier.selectBrush().setSelectMode(SelectMode::Connected);
				ctx->Yield();
				command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
				ctx->Yield();
				IM_CHECK(node->volume()->voxel(pos).getFlags() & voxel::FlagOutline);
				IM_CHECK_EQ(node->volume()->voxel(otherSlice).getFlags(), 0);
				ctx->Yield(3);
			}
			_sceneMgr->modifier().shapeBrush().setRadius(0);
			_sceneMgr->modifier().setBrushType(BrushType::Shape);
		};

	IM_REGISTER_TEST(engine, testCategory(), "target crosses follow all planes")->TestFunc =
		[=](ImGuiTestContext *ctx) {
			IM_CHECK(prepareUITest(ctx));
			util::ScopedVarChange visible(cfg::VoxEditShowSlices, "true");
			const voxel::Region region(glm::ivec3(-3, -4, -5), glm::ivec3(4, 3, 2));
			IM_CHECK(_sceneMgr->newScene(true, "slice targets", region));
			ctx->Yield();
			IM_CHECK(focusWindow(ctx, id));
			const char *canvases[] = {"**/backcanvas", "**/rightcanvas", "**/topcanvas"};
			for (int view = 0; view < 3; ++view) {
				ctx->MouseMove(canvases[view]);
				ctx->Yield();
				const TargetView &hovered = _targetViews[view];
				IM_CHECK(hovered.draw != nullptr);
				ctx->MouseMoveToPos(
					ImVec2(hovered.origin.x + 2.5f * hovered.cell, hovered.origin.y + 3.5f * hovered.cell));
				ctx->Yield();
				IM_CHECK(_target == voxelPosition(view, region, _slices[view], 2, 3));
				for (int projectedView = 0; projectedView < 3; ++projectedView) {
					const TargetView &targetView = _targetViews[projectedView];
					IM_CHECK(targetView.draw != nullptr);
					const glm::ivec2 projected = viewPosition(projectedView, region, _target);
					const int depthAxes[] = {2, 0, 1};
					IM_CHECK(voxelPosition(projectedView, region, _target[depthAxes[projectedView]], projected.x,
										   projected.y) == _target);
					bool left = false, right = false, top = false, bottom = false;
					for (const ImDrawVert &vertex : targetView.draw->VtxBuffer) {
						if (vertex.col != ImGui::GetColorU32(ImVec4(style::color(style::ColorBrushGizmoLine)))) {
							continue;
						}
						left |= vertex.pos.x <= targetView.min.x + 2.0f;
						right |= vertex.pos.x >= targetView.max.x - 2.0f;
						top |= vertex.pos.y <= targetView.min.y + 2.0f;
						bottom |= vertex.pos.y >= targetView.max.y - 2.0f;
					}
					IM_CHECK(left && right && top && bottom);
				}
			}
			const glm::ivec3 lastTarget = _target;
			ctx->MouseMoveToPos(ImVec2(0, 0));
			ctx->Yield();
			IM_CHECK(_target == lastTarget);
			IM_CHECK_EQ(voxelutil::countVoxels(*_sceneMgr->sceneGraph().node(_sceneMgr->activeNode()).volume()), 0);
		};

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
			ctx->MouseDown(ImGuiMouseButton_Left);
			command::executeCommands("+actionexecute 1 1");
			ctx->Yield();
			command::executeCommands("-actionexecute 1 1");
			ctx->MouseUp(ImGuiMouseButton_Left);
			ctx->Yield();
			voxel::RawVolume *volume = _sceneMgr->volume(_sceneMgr->activeNodeUUID());
			IM_CHECK_EQ(volume->voxel(pos).getColor(), 42);
			IM_CHECK(!voxel::isAir(volume->voxel(pos).getMaterial()));
			IM_CHECK(_sceneMgr->undo());
			IM_CHECK(voxel::isAir(volume->voxel(pos).getMaterial()));
			IM_CHECK(_sceneMgr->redo());
			IM_CHECK_EQ(volume->voxel(pos).getColor(), 42);
			command::executeCommands("+actionexecutedelete 1 1");
			ctx->Yield();
			command::executeCommands("-actionexecutedelete 1 1");
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
			command::executeCommands("+actionexecute 1 1");
			ctx->Yield();
			ctx->MouseMoveToPos(ImVec2(origin.x + 6.5f * cell, origin.y + 2.5f * cell));
			command::executeCommands("-actionexecute 1 1");
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
		_sceneMgr->modifier().setSliceMode(true);
		_sceneMgr->modifier().setSliceRegion(voxel::Region(pos, pos));
		_sceneMgr->modifier().setCursorPosition(pos, voxel::FaceNames::Back);
		command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
		IM_CHECK(voxel::isAir(node->volume()->voxel(pos).getMaterial()));
		node->setLocked(false);
		const voxel::Voxel selected = voxel::createVoxel(voxel::VoxelType::Generic, 7, NO_NORMAL, voxel::FlagOutline);
		IM_CHECK(node->volume()->setVoxel(pos, selected));
		const glm::ivec3 otherPos = pos + glm::ivec3(1, 0, 0);
		IM_CHECK(node->volume()->setVoxel(otherPos, voxel::createVoxel(voxel::VoxelType::Generic, 8)));
		_sceneMgr->modified(node->uuid(), region);
		_sceneMgr->modifier().setSliceRegion(voxel::Region(pos, otherPos));
		_sceneMgr->modifier().setModifierType(ModifierType::Override);
		_sceneMgr->modifier().setCursorPosition(pos, voxel::FaceNames::Back);
		command::executeCommands("+actionexecute 1 1");
		_sceneMgr->modifier().setCursorPosition(otherPos, voxel::FaceNames::Back);
		command::executeCommands("-actionexecute 1 1");
		IM_CHECK_EQ(node->volume()->voxel(pos).getColor(), 42);
		IM_CHECK_EQ(node->volume()->voxel(pos).getFlags(), voxel::FlagOutline);
		IM_CHECK_EQ(node->volume()->voxel(otherPos).getColor(), 8);
		_sceneMgr->modifier().setSliceMode(false);
	};
}

} // namespace voxedit
