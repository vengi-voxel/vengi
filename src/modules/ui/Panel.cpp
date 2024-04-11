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

#ifdef IMGUI_ENABLE_TEST_ENGINE
void Panel::registerUITests(ImGuiTestEngine *, const char *) {
	Log::warn("No tests registered for panel %s", _title.c_str());
}

const char *Panel::testCategory() const {
	return _app->appname().c_str();
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

#endif

} // namespace ui
