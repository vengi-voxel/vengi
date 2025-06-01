/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "ui/TextEditor.h"
#include "voxelgenerator/LUAApi.h"
#include "voxelui/LUAApiWidget.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

/**
 * @brief LUA script integration panel
 */
class ScriptPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	TextEditor _textEditor;
	SceneManagerPtr _sceneMgr;
	voxelui::LUAApiWidget _luaApiWidget;
	bool _scriptEditor = false;
	core::String _activeScriptFilename;
	voxelgenerator::LUAScript _luaScript;

public:
	ScriptPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "script"), _sceneMgr(sceneMgr) {
	}
	void update(const char *id, command::CommandExecutionListener &listener);

	bool updateEditor(const char *id);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
