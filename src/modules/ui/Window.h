#pragma once

#include "TurboBadger.h"

namespace ui {

class UIApp;

class Window: public tb::TBWindow {
public:
	Window(UIApp* app);
	Window(Window* parent);
	virtual ~Window() {}

	bool loadResourceFile(const char *filename);
	void loadResourceData(const char *data);
	void loadResource(tb::TBNode &node);

	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
