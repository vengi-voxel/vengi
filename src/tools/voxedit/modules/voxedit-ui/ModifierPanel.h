/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "math/Axis.h"

namespace voxedit {

class ModifierPanel {
private:
	bool mirrorAxisRadioButton(const char *title, math::Axis type);
	void addShapes();
	void addMirrorPlanes();
	void addModifierModes();
	void addModifiers(command::CommandExecutionListener &listener);
public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

}
