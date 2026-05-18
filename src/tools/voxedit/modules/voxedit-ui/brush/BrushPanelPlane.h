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
 * @brief Settings UI for the plane brush (place, erase, and override on a plane).
 */
class BrushPanelPlane {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
};

} // namespace voxedit
