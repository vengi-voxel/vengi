/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "command/CommandHandler.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec4.hpp>

namespace scenegraph {
class SceneGraphNode;
}

namespace palette {
class Palette;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class PalettePanel : public ui::Panel {
private:
	using Super = ui::Panel;

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
	core::String _lospecID;
	core::DynamicArray<core::String> _availablePalettes;
	SceneManagerPtr _sceneMgr;

	void closestColor(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void createPopups(scenegraph::SceneGraphNode &node);

	void handleContextMenu(uint8_t uiIdx, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener,
				   palette::Palette &palette);
	void handleDragAndDrop(uint8_t palIdx, uint8_t uiIdx, scenegraph::SceneGraphNode &node, palette::Palette &palette);
	void addColor(float startingPosX, uint8_t palIdx, uint8_t uiIdx, scenegraph::SceneGraphNode &node,
				  command::CommandExecutionListener &listener);
	bool showColorPicker(uint8_t palIdx, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void reloadAvailablePalettes();

	uint8_t currentSceneColor() const;
	uint8_t currentPaletteIndex() const;
public:
	PalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr);
	void update(const char *title, command::CommandExecutionListener &listener);
	bool hasFocus() const;
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *title) override;
#endif
};

}
