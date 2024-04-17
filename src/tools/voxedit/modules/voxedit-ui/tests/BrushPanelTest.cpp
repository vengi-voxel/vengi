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

		// by activating the edit mode viewport - we activate the brush panel
		const core::String id = Viewport::viewportId(viewportId);
		ctx->ItemClick(id.c_str());

		// now we can focus the brush panel
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

	IM_REGISTER_TEST(engine, testCategory(), "select")->TestFunc = [=](ImGuiTestContext *ctx) {
		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::None);
		modifier.setModifierType(ModifierType::Select);

		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);

		const core::String id = Viewport::viewportId(viewportId);
		ctx->ItemClick(id.c_str());

		ImGuiWindow* window = ImGui::FindWindowByName(id.c_str());
		IM_CHECK_SILENT(window != nullptr);
		ctx->MouseMoveToPos(window->Rect().GetCenter());

		modifier.unselect();
		IM_CHECK(modifier.selections().empty());

		command::executeCommands("+actionexecute 1 1");
		ctx->MouseMoveToPos(window->Rect().GetTL());
		command::executeCommands("-actionexecute 1 1");
		IM_CHECK(!modifier.selections().empty());

		modifier.unselect();
	};
	// TODO: copy and paste
}

} // namespace voxedit
