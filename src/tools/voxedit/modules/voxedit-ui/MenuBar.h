/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "voxelui/ScriptBrowserPanel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
class OptionsPanel;

class MenuBar : public ui::Panel {
private:
	using Super = ui ::Panel;
	SceneManagerPtr _sceneMgr;
	OptionsPanel *_optionsPanel;
	voxelui::ScriptBrowserPanel *_scriptBrowserPanel;

public:
	MenuBar(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, OptionsPanel *optionsPanel,
			voxelui::ScriptBrowserPanel *scriptBrowserPanel)
		: Super(app, "menubar"), _sceneMgr(sceneMgr), _optionsPanel(optionsPanel),
		  _scriptBrowserPanel(scriptBrowserPanel) {
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
