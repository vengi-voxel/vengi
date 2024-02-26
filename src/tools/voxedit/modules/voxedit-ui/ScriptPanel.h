/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "ui/TextEditor.h"
#include "voxelgenerator/LUAGenerator.h"

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
	core::String _activeScript;
	core::String _activeScriptFilename;
	int _currentScript = -1;
	core::DynamicArray<voxelgenerator::LUAScript> _scripts;
	core::DynamicArray<voxelgenerator::LUAParameterDescription> _scriptParameterDescription;
	core::DynamicArray<core::String> _enumValues;
	core::DynamicArray<core::String> _scriptParameters;
	SceneManagerPtr _sceneMgr;
	bool _scriptEditor = false;

	bool updateScriptExecutionPanel(command::CommandExecutionListener &listener);
	void updateScriptPanel(command::CommandExecutionListener &listener);
	void reloadScriptParameters(const core::String &script);

public:
	ScriptPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app), _sceneMgr(sceneMgr) {
	}
	void update(const char *title, command::CommandExecutionListener &listener);

	bool updateEditor(const char *title);
};

} // namespace voxedit
