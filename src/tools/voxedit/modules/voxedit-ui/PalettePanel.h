/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace voxedit {

class PalettePanel {
private:
	core::String _currentSelectedPalette;
	core::DynamicArray<core::String> _availablePalettes;
	void reloadAvailablePalettes();
public:
	PalettePanel();
	void update(const char *title, command::CommandExecutionListener &listener);
};

}
