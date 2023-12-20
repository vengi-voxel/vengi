/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/TextEditor.h"
#include "voxelgenerator/LUAGenerator.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

/**
 * @brief LUA script integration panel
 */
class ScriptPanel {
private:
	TextEditor _textEditor;
	core::String _activeScript;
	core::String _activeScriptFilename;
	int _currentScript = -1;
	core::DynamicArray<voxelgenerator::LUAScript> _scripts;
	core::DynamicArray<voxelgenerator::LUAParameterDescription> _scriptParameterDescription;
	core::DynamicArray<core::String> _enumValues;
	core::DynamicArray<core::String> _scriptParameters;
	bool _scriptEditor = false;

	bool updateScriptExecutionPanel(command::CommandExecutionListener &listener);
	void updateScriptPanel(command::CommandExecutionListener &listener);
	void reloadScriptParameters(const core::String &script);

public:
	void update(const char *title, command::CommandExecutionListener &listener);

	bool updateEditor(const char *title, ui::IMGUIApp *app);
};

} // namespace voxedit