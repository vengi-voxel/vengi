/**
 * @file
 */

#include "../BrushPanel.h"
#include "ui/IMGUIApp.h"
#include "command/CommandHandler.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

static int viewportEditMode(ImGuiTestContext *ctx, ui::IMGUIApp *app) {
	int viewportId = -1;
	for (int i = 0; i < 8; ++i) {
		const core::String &viewportTitle = Viewport::viewportId(i, true);
		Viewport *viewport = (Viewport*)app->getPanel(viewportTitle);
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

void BrushPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "cycle brush types")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);

		const core::String id = Viewport::viewportId(viewportId);
		ctx->ItemClick(id.c_str());
		IM_CHECK(focusWindow(ctx, title));

		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			const core::String id = core::String::format("brushes/###button%d", i);
			ctx->ItemClick(id.c_str());
			ctx->Yield();
			const BrushType brushType = modifier.brushType();
			IM_CHECK_EQ((int)brushType, i);
		}
	};

	// TODO: select
	// TODO: copy and paste
}

} // namespace voxedit
