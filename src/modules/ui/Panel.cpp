/**
 * @file
 */

#include "Panel.h"
#include "IMGUIApp.h"
#include "core/Log.h"
#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "core/Assert.h"
#endif

namespace ui {

Panel::Panel(IMGUIApp *app, const char *title) : _app(app), _title(title) {
	Log::debug("create panel %s", _title.c_str());
	_app->addPanel(this);
}

Panel::~Panel() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
	unregisterUITests(_app->imguiTestEngine());
#endif
	_app->removePanel(this);
}

#ifdef IMGUI_ENABLE_TEST_ENGINE
void Panel::registerUITests(ImGuiTestEngine *, const char *) {
	Log::warn("No tests registered for panel %s", _title.c_str());
}

bool Panel::saveFile(ImGuiTestContext *ctx, const char *filename) {
	ImGuiContext& g = *ctx->UiContext;
	IM_CHECK_RETV(focusWindow(ctx, "Save file"), false);
	ctx->ItemInputValue("Filename", filename);
	ctx->Yield();
	const int currentPopupSize = g.OpenPopupStack.Size;
	IM_CHECK_RETV(currentPopupSize > 0, false);
	ctx->ItemClick("Save");
	ctx->Yield();
	if (g.OpenPopupStack.Size == currentPopupSize) {
		IM_CHECK_RETV(g.OpenPopupStack[g.OpenPopupStack.Size - 1].PopupId == ctx->GetID("###fileoverwritepopup"), false);
		IM_CHECK_RETV(focusWindow(ctx, "###fileoverwritepopup"), false);
		ctx->ItemClick("Yes");
		ctx->Yield();
	}

	return true;
}

bool Panel::focusWindow(ImGuiTestContext *ctx, const char *title) {
	IM_CHECK_SILENT_RETV(title != nullptr, false);
	ImGuiWindow* window = ImGui::FindWindowByName(title);
	if (window == nullptr) {
		ctx->LogError("Error: could not find window with title/id %s", title);
		IM_CHECK_SILENT_RETV(window != nullptr, false);
	}
	ctx->WindowFocus(window->ID);
	ctx->SetRef(window);
	return true;
}

void Panel::unregisterUITests(ImGuiTestEngine *engine) {
	// https://github.com/ocornut/imgui_test_engine/issues/46
	while (ImGuiTest* test = ImGuiTestEngine_FindTestByName(engine, testCategory(), nullptr)) {
		ImGuiTestEngine_UnregisterTest(engine, test);
	}
}

#endif

} // namespace ui
