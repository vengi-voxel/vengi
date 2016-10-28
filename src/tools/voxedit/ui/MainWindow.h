/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "../VoxEdit.h"

/**
 * @brief Voxel editing tools panel
 */
class MainWindow: public ui::Window {
public:
	MainWindow(VoxEdit* tool) :
			ui::Window(tool) {
		SetSettings(tb::WINDOW_SETTINGS_CAN_ACTIVATE);
		core_assert_always(loadResourceFile("ui/window/main.tb.txt"));
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		return ui::Window::OnEvent(ev);
	}
};
