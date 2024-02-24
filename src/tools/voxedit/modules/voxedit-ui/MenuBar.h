/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "core/String.h"
#include "core/collection/RingBuffer.h"
#include "command/CommandHandler.h"

namespace voxedit {

using LastOpenedFiles = core::RingBuffer<core::String, 10>;

class MenuBar : public ui::Panel {
private:
	LastOpenedFiles _lastOpenedFiles;
public:
	PANEL_CLASS(MenuBar)
	void setLastOpenedFiles(const LastOpenedFiles &lastOpenedFiles);
	/**
	 * @return true if the dock layout should get reset
	 */
	void colorReductionOptions();
	bool update(ui::IMGUIApp *app, command::CommandExecutionListener &listener);
};

inline void MenuBar::setLastOpenedFiles(const LastOpenedFiles &lastOpenedFiles) {
	_lastOpenedFiles = lastOpenedFiles;
}

}
