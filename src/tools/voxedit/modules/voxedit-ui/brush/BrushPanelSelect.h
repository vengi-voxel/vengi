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
 * @brief Settings UI for the select brush (selection modes, bounds editing, and actions).
 */
class BrushPanelSelect {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);

private:
	void handleSelectBox3D(BrushPanelContext &ctx, int nodeId);
	void handleSelectCircle(BrushPanelContext &ctx, int nodeId);
	void handleSelectPaint(BrushPanelContext &ctx, int nodeId);
	void handleSelectFuzzyColor(BrushPanelContext &ctx);
	void handleSelectFlatSurface(BrushPanelContext &ctx);
	void handleSelectLasso(command::CommandExecutionListener &listener);
};

} // namespace voxedit
