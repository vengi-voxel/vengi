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
class ToolWindow: public ui::Window {
public:
	ToolWindow(VoxEdit* tool) :
			ui::Window(tool) {
		core_assert_always(loadResourceFile("ui/window/tool.tb.txt"));
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		return ui::Window::OnEvent(ev);
	}
};
