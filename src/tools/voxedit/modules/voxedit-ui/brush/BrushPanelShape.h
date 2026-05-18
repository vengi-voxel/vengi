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
 * @brief Settings UI for the shape brush (primitive placement modes and AABB options).
 */
class BrushPanelShape {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);

private:
	void addShapes(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
};

} // namespace voxedit
