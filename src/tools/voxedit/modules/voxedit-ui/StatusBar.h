/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace voxedit {

class StatusBar {
public:
	void update(const char *title, float height, const core::String &lastExecutedCommand);
};

}
