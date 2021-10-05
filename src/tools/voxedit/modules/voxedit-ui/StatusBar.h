/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace voxedit {

class StatusBar {
public:
	void update(const char *title, const core::String &lastExecutedCommand);
};

}
