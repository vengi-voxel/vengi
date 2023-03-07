/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec4.hpp>

namespace scenegraph {
class SceneGraphNode;
}

namespace voxel {
class Palette;
}

namespace voxedit {

class PalettePanel {
private:
	float _intensityChange = 0.0f;
	int _closestMatch = -1;
	glm::vec4 _closestColor{0.0f, 0.0f, 0.0f, 1.0f};
	bool _colorHovered = false; // caching the hover state per frame
	bool _hasFocus = false;
	bool _searchFittingColors = false;
	bool _colorPickerChange = false;
	bool _popupSwitchPalette = false;
	const uint32_t _redColor;
	const uint32_t _yellowColor;
	const uint32_t _darkRedColor;
	core::String _importPalette;
	core::String _currentSelectedPalette;
	core::DynamicArray<core::String> _availablePalettes;

	void closestColor(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void createPopups(scenegraph::SceneGraphNode &node);

	void addColor(float startingPosX, uint8_t palIdx, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	bool showColorPicker(uint8_t palIdx, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void reloadAvailablePalettes();

	uint8_t currentSceneColor() const;
	uint8_t currentPaletteIndex() const;
public:
	PalettePanel();
	void update(const char *title, command::CommandExecutionListener &listener);
	bool hasFocus() const;
};

}
