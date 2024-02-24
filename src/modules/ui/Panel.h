/**
 * @file
 */

#pragma once

namespace ui {

class IMGUIApp;

class Panel {
protected:
	IMGUIApp *_app;
	Panel(IMGUIApp *app) : _app(app) {
	}
};

#define PANEL_CLASS(name)                                                                                         \
private:                                                                                                               \
	using Super = ui::Panel;                                                                                           \
                                                                                                                       \
public:                                                                                                                \
	name(ui::IMGUIApp *app) : Super(app) {}

} // namespace ui
