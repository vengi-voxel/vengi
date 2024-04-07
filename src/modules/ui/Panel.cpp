/**
 * @file
 */

#include "Panel.h"
#include "IMGUIApp.h"
#include "core/Log.h"

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
#endif

} // namespace ui
