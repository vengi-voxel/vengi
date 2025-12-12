/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "ui/Panel.h"

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

	void paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener);
	void addColor(float startingPosX, uint8_t paletteColorIdx, float colorButtonSize, scenegraph::SceneGraphNode &node,
				  command::CommandExecutionListener &listener);

public:
	NormalPalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr);
	virtual ~NormalPalettePanel() = default;
	void update(const char *id, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
