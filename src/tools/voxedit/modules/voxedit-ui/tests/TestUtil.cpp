/**
 * @file
 */

#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "voxedit-ui/ViewMode.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

bool changeViewMode(ImGuiTestContext *ctx, ViewMode viewMode) {
	ImGuiWindow* window = ImGui::FindWindowByName("###app");
	if (window == nullptr) {
		ctx->LogError("Error: could not find ###app window");
		IM_CHECK_SILENT_RETV(window != nullptr, false);
	}
	ctx->SetRef(window);
	ctx->MenuClick("Help/Welcome screen");
	ctx->Yield();
	ctx->SetRef(POPUP_TITLE_WELCOME);
	const char *viewModeStr = getViewModeString(viewMode);
	const core::String &viewModeStrPath = core::String::format("View mode/%s", viewModeStr);
	ctx->ComboClick(viewModeStrPath.c_str());
	ctx->ItemClick("###Close");
	return true;
}

bool centerOnViewport(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId, ImVec2 offset) {
	IM_CHECK_RETV(viewportId != -1, false);
	ImGuiWindow *window = ImGui::FindWindowByName(Viewport::viewportId(viewportId).c_str());
	IM_CHECK_SILENT_RETV(window != nullptr, false);
	ImVec2 pos = window->Rect().GetCenter();
	pos.x += offset.x;
	pos.y += offset.y;
	// force tracing via mouse
	sceneMgr->setMousePos(0, 0);
	sceneMgr->setMousePos(1, 1);
	// reset the last trace to ensure that after placing the cursor in fast mode the trace is executed again
	sceneMgr->resetLastTrace();
	ctx->MouseMoveToPos(pos);
	ctx->Yield();
	return true;
}

int voxelCount(const SceneManagerPtr &sceneMgr, int node) {
	const int activeNode = node == InvalidNodeId ? sceneMgr->sceneGraph().activeNode() : node;
	scenegraph::SceneGraphNode *model = sceneMgr->sceneGraphModelNode(activeNode);
	IM_CHECK_RETV(model != nullptr, -1);
	const int cnt = voxelutil::countVoxels(*model->volume());
	IM_CHECK_RETV(cnt == 0, -1);
	return cnt;
}

void executeViewportClick() {
	command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
}

bool executeViewportClickArea(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId,
							  const ImVec2 &offset) {
	IM_CHECK_RETV(centerOnViewport(ctx, sceneMgr, viewportId, {0.0f, 0.0f}), false);
	command::executeCommands("+actionexecute 1 1");
	IM_CHECK_RETV(centerOnViewport(ctx, sceneMgr, viewportId, offset), false);
	command::executeCommands("-actionexecute 1 1");
	return true;
}

int viewportEditMode(ImGuiTestContext *ctx, ui::IMGUIApp *app) {
	int viewportId = -1;
	for (int i = 0; i < 8; ++i) {
		const core::String &viewportTitle = Viewport::viewportId(i, true);
		Viewport *viewport = (Viewport *)app->getPanel(viewportTitle);
		if (viewport == nullptr) {
			break;
		}
		if (!viewport->isSceneMode()) {
			viewportId = i;
			break;
		}
	}
	// not found, this means that the scene mode checkbox is set...
	// so let's uncheck it to make this a viewport in edit mode
	if (viewportId == -1) {
		viewportId = 0;
		const core::String viewportRef = Viewport::viewportId(viewportId) + "/##MenuBar/Scene Mode";
		ctx->ItemClick(viewportRef.c_str());
	}
	return viewportId;
}

int viewportSceneMode(ImGuiTestContext *ctx, ui::IMGUIApp *app) {
	int viewportId = -1;
	for (int i = 0; i < 8; ++i) {
		const core::String &viewportTitle = Viewport::viewportId(i, true);
		Viewport *viewport = (Viewport *)app->getPanel(viewportTitle);
		if (viewport == nullptr) {
			break;
		}
		if (viewport->isSceneMode()) {
			viewportId = i;
			break;
		}
	}
	// not found, this means that the scene mode checkbox is not set...
	// so let's check it to make this a viewport in scene mode
	if (viewportId == -1) {
		viewportId = 0;
		const core::String viewportRef = Viewport::viewportId(viewportId) + "/##MenuBar/Scene Mode";
		ctx->ItemClick(viewportRef.c_str());
	}
	return viewportId;
}

bool activateViewportSceneMode(ImGuiTestContext *ctx, ui::IMGUIApp *app) {
	const int viewportId = viewportSceneMode(ctx, app);
	IM_CHECK_RETV(viewportId != -1, false);
	const core::String vid = Viewport::viewportId(viewportId);
	ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
	if (window == nullptr) {
		ctx->LogError("Error: could not find viewport window with title/id %s", vid.c_str());
		IM_CHECK_SILENT_RETV(window != nullptr, false);
	}
	ctx->WindowFocus(window->ID);
	ctx->Yield();
	ctx->ItemClick(window->ID);
	ctx->Yield();
	return true;
}

bool activateViewportEditMode(ImGuiTestContext *ctx, ui::IMGUIApp *app) {
	const int viewportId = viewportEditMode(ctx, app);
	IM_CHECK_RETV(viewportId != -1, false);
	const core::String vid = Viewport::viewportId(viewportId);
	ImGuiWindow *window = ImGui::FindWindowByName(vid.c_str());
	if (window == nullptr) {
		ctx->LogError("Error: could not find viewport window with title/id %s", vid.c_str());
		IM_CHECK_SILENT_RETV(window != nullptr, false);
	}
	ctx->WindowFocus(window->ID);
	ctx->Yield();
	ctx->ItemClick(window->ID);
	ctx->Yield();
	return true;
}

bool setVoxel(const SceneManagerPtr &sceneMgr, scenegraph::SceneGraphNode *node, const glm::ivec3 &pos,
			  const voxel::Voxel &voxel) {
	voxel::RawVolume *volume = node->volume();
	IM_CHECK_RETV(volume != nullptr, false);
	IM_CHECK_RETV(volume->region().containsPoint(pos), false);
	IM_CHECK_RETV(volume->setVoxel(pos, voxel), false);
	sceneMgr->modified(node->id(), voxel::Region(pos, pos));
	return true;
}

} // namespace voxedit
