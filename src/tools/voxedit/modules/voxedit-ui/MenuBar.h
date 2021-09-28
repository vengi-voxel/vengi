/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"
#include "command/CommandHandler.h"

namespace voxedit {

class MenuBar {
private:
	bool actionMenuItem(const char *title, const char *command, command::CommandExecutionListener &listener);

public:
	void update(ui::imgui::IMGUIApp* app, command::CommandExecutionListener &listener);
	bool _popupSceneSettings = false;
};

}
