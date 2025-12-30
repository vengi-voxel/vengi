/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
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

	void paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void addColor(ImVec2 &cursorPos, float startingPosX, float contentRegionRightEdge, uint8_t paletteColorIdx,
				  float colorButtonSize, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	uint8_t currentSceneNormal() const;
	void setTargetNormal(const palette::NormalPalette &normalPalette, const glm::vec3 &normal);

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
