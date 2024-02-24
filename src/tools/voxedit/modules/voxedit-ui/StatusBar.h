/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "core/String.h"

namespace voxedit {

/**
 * @brief Status bar on to the bottom of the main window
 */
class StatusBar : public ui::Panel {
public:
	PANEL_CLASS(StatusBar)
	void update(const char *title, float height, const core::String &lastExecutedCommand);
};

}
