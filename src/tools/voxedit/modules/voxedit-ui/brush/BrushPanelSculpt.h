/**
 * @file
 */

#pragma once

#include "BrushPanelContext.h"
#include "core/String.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

/**
 * @brief Settings UI for the sculpt brush (sculpt modes, reskin, smoothing, and extend plane).
 */
class BrushPanelSculpt {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);

private:
	void executeSculptBrush(BrushPanelContext &ctx);
	void loadSkinFromFile(BrushPanelContext &ctx, const core::String &filename);
};

} // namespace voxedit
