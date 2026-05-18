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
 * @brief Settings UI for the normal brush (normal palette paint modes).
 */
class BrushPanelNormal {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
};

} // namespace voxedit
