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
 * @brief Settings UI for the extrude brush (face direction, depth, offsets, and fill walls).
 */
class BrushPanelExtrude {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);

private:
	void executeExtrudeBrush(BrushPanelContext &ctx);
};

} // namespace voxedit
