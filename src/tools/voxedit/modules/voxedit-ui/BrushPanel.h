/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/TextEditor.h"
#include "ui/IMGUIApp.h"
#include "voxel/Face.h"
#include "voxelgenerator/LUAGenerator.h"

namespace voxedit {

/**
 * @brief LUA script integration panel
 */
class BrushPanel {
private:
	TextEditor _textEditor;
	core::String _activeScript;
	core::String _activeScriptFilename;
	int _currentScript = -1;
	core::String _stamp;
	int _stampPaletteIndex = 0;
	voxel::FaceNames _face = voxel::FaceNames::PositiveX;
	core::DynamicArray<voxelgenerator::LUAScript> _scripts;
	core::DynamicArray<voxelgenerator::LUAParameterDescription> _scriptParameterDescription;
	core::DynamicArray<core::String> _enumValues;
	core::DynamicArray<core::String> _scriptParameters;
	bool _scriptEditor = false;

	void addModifiers(command::CommandExecutionListener &listener);
	void brushRegion();
	void brushSettings(command::CommandExecutionListener &listener);

	void addShapes(command::CommandExecutionListener &listener);
	void addMirrorPlanes(command::CommandExecutionListener &listener);
	bool mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener);

	void updateScriptBrushPanel(command::CommandExecutionListener &listener);
	void updateShapeBrushPanel(command::CommandExecutionListener &listener);
	void stampBrushUseSelection(scenegraph::SceneGraphNode &node, voxel::Palette &palette);
	void stampBrushOptions(scenegraph::SceneGraphNode &node, voxel::Palette &palette, command::CommandExecutionListener &listener);
	void updateStampBrushPanel(command::CommandExecutionListener &listener);
	void reloadScriptParameters(const core::String& script);
public:
	void update(const char *title, command::CommandExecutionListener &listener);

	bool updateEditor(const char *title, ui::IMGUIApp* app);
};

}
