/**
 * @file
 */

#pragma once

#include "ui/imgui/TextEditor.h"
#include "ui/imgui/IMGUIApp.h"
#include "voxelgenerator/LUAGenerator.h"

namespace voxedit {

class ScriptPanel {
private:
	TextEditor _textEditor;
	core::String _activeScript;
	core::String _activeScriptFilename;
	int _currentScript = -1;
	core::DynamicArray<voxelgenerator::LUAScript> _scripts;
	core::DynamicArray<voxelgenerator::LUAParameterDescription> _scriptParameterDescription;
	core::DynamicArray<core::String> _scriptParameters;
	bool _scriptEditor = false;

	void reloadScriptParameters(const core::String& script);
public:
	void update(const char *title, const char *scriptEditorTitle, ui::imgui::IMGUIApp* app);
};

}
