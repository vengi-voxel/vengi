/**
 * @file
 */

#pragma once

#include "core/Log.h"

namespace ui {

class IMGUIApp;

class Panel {
protected:
	IMGUIApp *_app;
	Panel(IMGUIApp *app, const char *title) : _app(app) {
		Log::debug("create panel %s", title);
	}
};

#define PANEL_CLASS(name)                                                                                         \
private:                                                                                                               \
	using Super = ui::Panel;                                                                                           \
                                                                                                                       \
public:                                                                                                                \
	name(ui::IMGUIApp *app) : Super(app, #name) {}

} // namespace ui
