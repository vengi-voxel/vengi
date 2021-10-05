/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace voxedit {

class StatusBar {
public:
	void update(const char *title, float height, const core::String &lastExecutedCommand);
};

}
