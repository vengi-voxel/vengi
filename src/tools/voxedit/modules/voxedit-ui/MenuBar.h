/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "core/String.h"
#include "core/collection/RingBuffer.h"
#include "command/CommandHandler.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

using LastOpenedFiles = core::RingBuffer<core::String, 10>;

class MenuBar : public ui::Panel {
private:
	using Super = ui ::Panel;
	LastOpenedFiles _lastOpenedFiles;
	SceneManagerPtr _sceneMgr;

public:
	MenuBar(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "menubar"), _sceneMgr(sceneMgr) {
	}
	void setLastOpenedFiles(const LastOpenedFiles &lastOpenedFiles);
	/**
	 * @return true if the dock layout should get reset
	 */
	void colorReductionOptions();
	static void viewportOptions();
	bool update(ui::IMGUIApp *app, command::CommandExecutionListener &listener);
};

inline void MenuBar::setLastOpenedFiles(const LastOpenedFiles &lastOpenedFiles) {
	_lastOpenedFiles = lastOpenedFiles;
}

}
