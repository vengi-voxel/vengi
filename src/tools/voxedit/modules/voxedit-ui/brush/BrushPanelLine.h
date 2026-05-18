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
 * @brief Settings UI for the line brush (continuous, bezier, thickness, sag, and stipple).
 */
class BrushPanelLine {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
};

} // namespace voxedit
