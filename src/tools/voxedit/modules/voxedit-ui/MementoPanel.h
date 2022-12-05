/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "image/Image.h"
#include "math/Axis.h"

namespace voxedit {

class MementoPanel {
public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

} // namespace voxedit
