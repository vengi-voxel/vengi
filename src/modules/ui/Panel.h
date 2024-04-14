/**
 * @file
 */

#pragma once

#include "core/String.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "ui/dearimgui/imgui_test_engine/imgui_te_engine.h"
#include "ui/dearimgui/imgui_test_engine/imgui_te_context.h"
#endif

namespace ui {

class IMGUIApp;

class Panel {
protected:
	IMGUIApp *_app;
	core::String _title;
	Panel(IMGUIApp *app, const char *title);

public:
	virtual ~Panel();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	virtual void registerUITests(ImGuiTestEngine *, const char *);
	void unregisterUITests(ImGuiTestEngine *);
	const char *testCategory() const { return _title.c_str(); }

	bool focusWindow(ImGuiTestContext *ctx, const char *title);
#endif
};

#define PANEL_CLASS(name)                                                                                         \
private:                                                                                                               \
	using Super = ui::Panel;                                                                                           \
                                                                                                                       \
public:                                                                                                                \
	name(ui::IMGUIApp *app) : Super(app, #name) {}

} // namespace ui
