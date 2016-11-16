/**
 * @file
 */

#pragma once

#include "ui/Window.h"

namespace voxedit {

class LSystemWindow: public ui::Window {
private:
	using Super = ui::Window;
public:
	LSystemWindow(ui::Window* window);

	bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
