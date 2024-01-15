/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Face.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class BrushPanel {
private:
	core::String _stamp;
	int _stampPaletteIndex = 0;
	voxel::FaceNames _face = voxel::FaceNames::PositiveX;

	void addModifiers(command::CommandExecutionListener &listener);
	void brushSettings(command::CommandExecutionListener &listener);

	void addShapes(command::CommandExecutionListener &listener);
	void addMirrorPlanes(command::CommandExecutionListener &listener);
	bool mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener);

	void updateShapeBrushPanel(command::CommandExecutionListener &listener);
	void stampBrushUseSelection(scenegraph::SceneGraphNode &node, palette::Palette &palette);
	void stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette, command::CommandExecutionListener &listener);
	void updateStampBrushPanel(command::CommandExecutionListener &listener);
	void updatePlaneBrushPanel(command::CommandExecutionListener &listener);
public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

}
