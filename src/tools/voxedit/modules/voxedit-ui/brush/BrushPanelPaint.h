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
 * @brief Settings UI for the paint brush (paint modes, factor, plane, gradient, and AABB options).
 */
class BrushPanelPaint {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
};

} // namespace voxedit
