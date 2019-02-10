/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"

namespace voxedit {

class NoiseWindow : public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;

	tb::TBEditField* _octaves;
	tb::TBEditField* _frequency;
	tb::TBEditField* _lacunarity;
	tb::TBEditField* _gain;
public:
	NoiseWindow(ui::turbobadger::Window* window);

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

}
