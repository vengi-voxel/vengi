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
		core::String input = "example";
		int size = 16;
		int spacing = 0;
		int thickness = 1;
	} _text;
	void updateSceneMode(command::CommandExecutionListener &listener);
	void updateEditMode(command::CommandExecutionListener &listener);
public:
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

} // namespace voxedit
