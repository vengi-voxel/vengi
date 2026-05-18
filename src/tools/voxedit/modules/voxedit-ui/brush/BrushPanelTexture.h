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
 * @brief Settings UI for the texture brush (image, UVs, projection, and UV editor popup).
 */
class BrushPanelTexture {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
	void createPopups(BrushPanelContext &ctx, command::CommandExecutionListener &listener);
};

} // namespace voxedit
