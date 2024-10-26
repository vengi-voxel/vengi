/**
 * @file
 */

#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "voxedit-ui/Viewport.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool centerOnViewport(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId, ImVec2 offset) {
	IM_CHECK_RETV(viewportId != -1, false);
	ImGuiWindow* window = ImGui::FindWindowByName(Viewport::viewportId(viewportId).c_str());
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
	const int cnt = voxelutil::visitVolume(*model->volume(), voxelutil::EmptyVisitor(), voxelutil::SkipEmpty());
	IM_CHECK_RETV(cnt == 0, -1);
	return cnt;
}

void executeViewportClick() {
	command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
}

bool executeViewportClickArea(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId, const ImVec2 &offset) {
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
		const core::String viewportRef = Viewport::viewportId(viewportId) + "/##menubar/Scene Mode";
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
		const core::String viewportRef = Viewport::viewportId(viewportId) + "/##menubar/Scene Mode";
		ctx->ItemClick(viewportRef.c_str());
	}
	return viewportId;
}

} // namespace voxedit
