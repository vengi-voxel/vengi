/**
 * @file
 */

#include "ui/IMGUIApp.h"
#include "voxedit-ui/Viewport.h"

namespace voxedit {

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
		const core::String viewportRef = Viewport::viewportId(viewportId) + "/##menubar/Edit Mode";
		ctx->ItemClick(viewportRef.c_str());
	}
	return viewportId;
}

} // namespace voxedit
