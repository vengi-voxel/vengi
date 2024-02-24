/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "command/CommandHandler.h"

namespace voxedit {

class ToolsPanel : public ui::Panel {
private:
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
	PANEL_CLASS(ToolsPanel)
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

} // namespace voxedit
