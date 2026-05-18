/**
 * @file
 */

#pragma once

#include "BrushPanelContext.h"
#include "core/String.h"

namespace command {
struct CommandExecutionListener;
}

namespace scenegraph {
class SceneGraphNode;
}

namespace palette {
class Palette;
}

namespace voxedit {

/**
 * @brief Settings UI for the stamp brush (model import, offset, rotation, and palette replace).
 */
class BrushPanelStamp {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);

private:
	core::String _stamp;
	int _stampPaletteIndex = 0;

	void stampBrushUseSelection(BrushPanelContext &ctx, scenegraph::SceneGraphNode &node, palette::Palette &palette,
								command::CommandExecutionListener &listener);
	void stampBrushOptions(BrushPanelContext &ctx, scenegraph::SceneGraphNode &node, palette::Palette &palette,
						   command::CommandExecutionListener &listener);
};

} // namespace voxedit
