/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace voxedit {

/**
 * @brief Status bar on to the bottom of the main window
 */
class StatusBar {
public:
	void update(const char *title, float height, const core::String &lastExecutedCommand);
};

}
