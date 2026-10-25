/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "color/RGBA.h"
#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "core/UUID.h"
#include "core/Var.h"
#include "ui/Panel.h"
#include "ui/dearimgui/imgui.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace scenegraph {
class SceneGraphNode;
}

namespace palette {
class NormalPalette;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class NormalPalettePanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	bool _recalcAll = false;
	bool _onlySurfaceVoxels = true;
	int _selectedIndex = NO_NORMAL;
	const uint32_t _redColor;
	const uint32_t _yellowColor;
	const uint32_t _darkRedColor;
	core::VarPtr _renderNormals;
	glm::vec3 _targetNormal{0.0f, 1.0f, 0.0f};
	bool _targetValid = true;
	bool _hasCopiedNormal = false;
	bool _normalChangePending = false;
	core::UUID _editingNodeUUID;
	color::RGBA _copiedNormal;
	core::UUID _lastNodeUUID;
	uint32_t _lastPaletteHash = 0;

	void paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void addColor(ImVec2 &cursorPos, float startingPosX, float contentRegionRightEdge, uint8_t paletteColorIdx,
				  float colorButtonSize, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void selectNormal(const palette::NormalPalette &palette, uint8_t index);
	void handleContextMenu(uint8_t index, scenegraph::SceneGraphNode &node);
	bool applyNormal(scenegraph::SceneGraphNode &node);
	void finishNormalEdit();
	uint8_t currentSceneNormal() const;
	void setTargetNormal(const glm::vec3 &normal);
	void drawNormalVisualization(const palette::NormalPalette &normalPalette);

public:
	NormalPalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr);
	virtual ~NormalPalettePanel() = default;
	void init();
	void update(const char *id, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
