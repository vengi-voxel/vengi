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

void Panel::focusWindow(ImGuiTestContext *ctx, const char *title) {
	core_assert(title != nullptr);
	ImGuiWindow* window = ImGui::FindWindowByName(title);
	IM_CHECK_SILENT(window != nullptr);
	if (window) {
		ctx->WindowFocus(window->ID);
	}
}

#endif

} // namespace ui
