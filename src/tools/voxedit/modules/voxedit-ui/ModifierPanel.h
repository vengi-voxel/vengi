/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "math/Axis.h"

namespace voxedit {

class ModifierPanel {
private:
	bool mirrorAxisRadioButton(const char *title, math::Axis type);

public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

}
