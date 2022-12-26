/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec4.hpp>

namespace voxelformat {
class SceneGraphNode;
}

namespace voxedit {

class PalettePanel {
private:
	float _intensityChange = 0.0f;
	int _closestMatch = -1;
	glm::vec4 _closestColor{0.0f, 0.0f, 0.0f, 1.0f};
	bool _pickerWheel = false;

	core::String _currentSelectedPalette;
	core::DynamicArray<core::String> _availablePalettes;
	bool _hasFocus = false;
	bool _searchFittingColors = false;

	void showColorPicker(uint8_t palIdx, voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void reloadAvailablePalettes();
public:
	PalettePanel();
	void update(const char *title, command::CommandExecutionListener &listener);
	bool hasFocus() const;
};

}
