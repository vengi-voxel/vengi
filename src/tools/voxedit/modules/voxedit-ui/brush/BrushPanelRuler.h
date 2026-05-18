/**
 * @file
 */

#pragma once

#include "BrushPanelContext.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

/**
 * @brief Settings UI for the ruler brush (reference position and distance measurement).
 */
class BrushPanelRuler {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
};

} // namespace voxedit
