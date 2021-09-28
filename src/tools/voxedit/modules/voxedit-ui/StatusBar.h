/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace voxedit {

class StatusBar {
public:
	void update(const core::String &lastExecutedCommand);
};

}
