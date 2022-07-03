/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/RingBuffer.h"
#include "ui/imgui/IMGUIApp.h"
#include "command/CommandHandler.h"

namespace voxedit {

using LastOpenedFiles = core::RingBuffer<core::String, 10>;

class MenuBar {
private:
	LastOpenedFiles _lastOpenedFiles;
	bool actionMenuItem(const char *title, const char *command, command::CommandExecutionListener &listener);

public:
	void setLastOpenedFiles(const LastOpenedFiles &lastOpenedFiles);
	/**
	 * @return true if the dock layout should get reset
	 */
	bool update(ui::imgui::IMGUIApp* app, command::CommandExecutionListener &listener);
	bool _popupSceneSettings = false;
};

inline void MenuBar::setLastOpenedFiles(const LastOpenedFiles &lastOpenedFiles) {
	_lastOpenedFiles = lastOpenedFiles;
}

}
