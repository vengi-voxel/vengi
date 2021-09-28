/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace voxedit {

class AnimationPanel {
public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

}
