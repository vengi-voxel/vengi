/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "image/Image.h"
#include "math/Axis.h"

namespace voxedit {

class ToolsPanel {
	struct Text {
		core::String font = "font.ttf";
		core::String input;
		int size = 16;
		int spacing = 0;
		int thickness = 1;
	} _text;
public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

} // namespace voxedit
