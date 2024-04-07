/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "core/String.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
struct ImGuiTestEngine;
#endif

namespace ui {

class IMGUIApp;

class Panel {
protected:
	IMGUIApp *_app;
	core::String _title;
	Panel(IMGUIApp *app, const char *title) : _app(app), _title(title) {
		Log::debug("create panel %s", _title.c_str());
	}

public:
	virtual ~Panel() {}
#ifdef IMGUI_ENABLE_TEST_ENGINE
	virtual bool registerUITests(ImGuiTestEngine *engine) {
		Log::warn("No tests registered for panel %s", _title.c_str());
		return false;
	}
#endif
};

#define PANEL_CLASS(name)                                                                                         \
private:                                                                                                               \
	using Super = ui::Panel;                                                                                           \
                                                                                                                       \
public:                                                                                                                \
	name(ui::IMGUIApp *app) : Super(app, #name) {}

} // namespace ui
