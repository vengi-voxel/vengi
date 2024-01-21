/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class AABBBrush;

class BrushPanel {
private:
	core::String _stamp;
	int _stampPaletteIndex = 0;

	void addModifiers(command::CommandExecutionListener &listener);
	void brushSettings(command::CommandExecutionListener &listener);

	void addShapes(command::CommandExecutionListener &listener);
	void addMirrorPlanes(command::CommandExecutionListener &listener, AABBBrush &brush);
	bool mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener, AABBBrush &brush);

	void stampBrushUseSelection(scenegraph::SceneGraphNode &node, palette::Palette &palette);
	void stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette, command::CommandExecutionListener &listener);
	void updateStampBrushPanel(command::CommandExecutionListener &listener);
	void updatePlaneBrushPanel(command::CommandExecutionListener &listener);
	void updateLineBrushPanel(command::CommandExecutionListener &listener);
	void updatePathBrushPanel(command::CommandExecutionListener &listener);

	void aabbBrushOptions(command::CommandExecutionListener &listener, AABBBrush &brush);
	void updateShapeBrushPanel(command::CommandExecutionListener &listener);
	void updatePaintBrushPanel(command::CommandExecutionListener &listener);
public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

}
