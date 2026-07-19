/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "voxelui/ScriptBrowserPanel.h"
#include "voxedit-ui/VoxBoxBrowserPanel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
class OptionsPanel;
class SceneDebugPanel;

class MenuBar : public ui::Panel {
private:
	using Super = ui ::Panel;
	SceneManagerPtr _sceneMgr;
	OptionsPanel *_optionsPanel;
	SceneDebugPanel *_sceneDebugPanel;
	voxelui::ScriptBrowserPanel *_scriptBrowserPanel;
	VoxBoxBrowserPanel *_voxBoxBrowserPanel;

public:
	MenuBar(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, OptionsPanel *optionsPanel,
			SceneDebugPanel *sceneDebugPanel, voxelui::ScriptBrowserPanel *scriptBrowserPanel,
			VoxBoxBrowserPanel *voxBoxBrowserPanel)
		: Super(app, "menubar"), _sceneMgr(sceneMgr), _optionsPanel(optionsPanel),
		  _sceneDebugPanel(sceneDebugPanel), _scriptBrowserPanel(scriptBrowserPanel),
		  _voxBoxBrowserPanel(voxBoxBrowserPanel) {
	}

	static void viewportOptions();
	static void viewModeOption();

	void init();
	bool update(ui::IMGUIApp *app, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override {
		// this is done in the main window tests
	}
#endif
};

} // namespace voxedit
