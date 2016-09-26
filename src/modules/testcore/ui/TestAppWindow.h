/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "../TestApp.h"

namespace testcore {

class TestAppWindow: public ui::Window {
private:
	TestApp* _application;

public:
	TestAppWindow(TestApp* application) :
			ui::Window(application), _application(application) {
		core_assert_always(loadResourceFile("ui/window/testapp.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
		m_mover.SetIgnoreInput(true);
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type == tb::EVENT_TYPE_CHANGED) {
			if (tb::TBSelectDropdown *select = GetWidgetByIDAndType<tb::TBSelectDropdown>(TBIDC("cammode"))) {
				const int value = select->GetValue();
				video::PolygonMode mode = video::PolygonMode::Solid;
				switch (value) {
				case 1:
					mode = video::PolygonMode::Points;
					break;
				case 2:
					mode = video::PolygonMode::WireFrame;
					break;
				default:
				case 0:
					break;
				}
				_application->camera().setPolygonMode(mode);
			}
		}
		return ui::Window::OnEvent(ev);
	}
};

}
