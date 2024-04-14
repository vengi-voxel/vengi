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
}

Panel::~Panel() {
}

#ifdef IMGUI_ENABLE_TEST_ENGINE
void Panel::registerUITests(ImGuiTestEngine *, const char *) {
	Log::warn("No tests registered for panel %s", _title.c_str());
}

bool Panel::focusWindow(ImGuiTestContext *ctx, const char *title) {
	IM_CHECK_SILENT_RETV(title != nullptr, false);
	ImGuiWindow* window = ImGui::FindWindowByName(title);
	if (window == nullptr) {
		ctx->LogError("Error: could not find window with title/id %s", title);
		IM_CHECK_SILENT_RETV(window != nullptr, false);
	}
	ctx->WindowFocus(window->ID);
	return true;
}

void Panel::unregisterUITests(ImGuiTestEngine *engine) {
	// https://github.com/ocornut/imgui_test_engine/issues/46
	// while (ImGuiTest* test = engine->ImGuiTestEngine_FindTestByName(engine, testCategory())) {
	// 	engine->TestsAll->erase(test);
	// }
}

#endif

} // namespace ui
