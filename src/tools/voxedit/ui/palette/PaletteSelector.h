/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Window.h"

namespace voxedit {

class PaletteSelector: public ui::turbobadger::Window {
private:
	using Super = ui::turbobadger::Window;
	tb::TBGenericStringItemSource _paletteList;
	std::string _currentSelectedPalette;
public:
	PaletteSelector(ui::turbobadger::Window* window);
	~PaletteSelector();

	bool onEvent(const tb::TBWidgetEvent &ev) override;
};

}
