/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "core/collection/Set.h"
#include "palette/PaletteCache.h"
#include "palette/PaletteView.h"
#include "ui/dearimgui/imgui.h"
#include "ui/Panel.h"
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

using PaletteSelection = core::Set<uint8_t>;

class PalettePanel : public ui::Panel {
private:
	using Super = ui::Panel;

	PaletteSelection _selectedIndices;
	int _selectedIndicesLast = -1;
	float _intensityChange = 0.0f;
	int _closestMatchPaletteColorIdx = -1;
	glm::vec4 _closestColor{0.0f, 0.0f, 0.0f, 1.0f};
	bool _colorHovered = false; // caching the hover state per frame
	bool _hasFocus = false;
	bool _searchFittingColors = false;
	bool _colorPickerChange = false;
	bool _popupSwitchPalette = false;
	const uint32_t _redColor;
	const uint32_t _yellowColor;
	const uint32_t _darkRedColor;
	int32_t _copyPaletteColorIdx = -1;
	core::String _importPalette;
	core::String _currentSelectedPalette;
	core::String _lospecID;
	palette::PaletteCache &_paletteCache;
	SceneManagerPtr _sceneMgr;

	void closestColor(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void createPopups(scenegraph::SceneGraphNode &node);

	void handleContextMenu(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
						   command::CommandExecutionListener &listener, palette::Palette &palette);
	void handleDragAndDrop(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node, palette::Palette &palette);
	void addColor(ImVec2 &cursorPos, float startingPosX, float contentRegionRightEdge,
				  uint8_t paletteColorIdx, uint8_t palettePanelIdx, float colorButtonSize, scenegraph::SceneGraphNode &node,
				  command::CommandExecutionListener &listener);
	bool showColorPicker(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
						 command::CommandExecutionListener &listener);
	void reloadAvailablePalettes();

	uint8_t currentSceneColor() const;
	uint8_t currentPaletteColorIndex() const;

public:
	PalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, palette::PaletteCache &paletteCache);
	void update(const char *id, command::CommandExecutionListener &listener);
	void onNewPaletteImport(const core::String& paletteName, bool setActive, bool searchBestColors);
	bool hasFocus() const;
	const PaletteSelection &selectedIndices() const;
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

inline const PaletteSelection &PalettePanel::selectedIndices() const {
	return _selectedIndices;
}

} // namespace voxedit
