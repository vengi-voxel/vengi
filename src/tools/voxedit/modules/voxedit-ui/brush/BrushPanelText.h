/**
 * @file
 */

#pragma once

#include "BrushPanelContext.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

/**
 * @brief Settings UI for the text brush (text input, font, spacing, and axis orientation).
 */
class BrushPanelText {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);

private:
	core::DynamicArray<core::String> _fontPaths;
	core::DynamicArray<core::String> _fontLabels;
	core::String _fontSearchFilter;
	bool _fontsLoaded = false;

	void loadFontList();
};

} // namespace voxedit
