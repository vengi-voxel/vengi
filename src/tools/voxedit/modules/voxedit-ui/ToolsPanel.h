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

class ToolsPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	struct Text {
		core::String font = "font.ttf";
		core::String input = "example";
		int size = 16;
		int spacing = 0;
		int thickness = 1;
	} _text;

	SceneManagerPtr _sceneMgr;

	void updateSceneMode(command::CommandExecutionListener &listener);
	void updateEditMode(command::CommandExecutionListener &listener);

public:
	ToolsPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "tools"), _sceneMgr(sceneMgr) {
	}
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

} // namespace voxedit
