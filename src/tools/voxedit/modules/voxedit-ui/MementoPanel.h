/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "command/CommandHandler.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class MementoPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;

public:
	MementoPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "memento"), _sceneMgr(sceneMgr) {
	}
	void update(const char *title, command::CommandExecutionListener &listener);
};

} // namespace voxedit
