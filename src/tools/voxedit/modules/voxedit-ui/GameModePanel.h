/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "core/Var.h"
#include "ui/Panel.h"

namespace voxedit {

class MainWindow;

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class GameModePanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	core::VarPtr _movementSpeed;
	core::VarPtr _jumpVelocity;
	core::VarPtr _bodyHeight;
	core::VarPtr _applyGravity;
	core::VarPtr _clipping;
	bool _gameModeEnabled = false;
	const MainWindow *_mainWindow = nullptr;

public:
	GameModePanel(ui::IMGUIApp *app, const MainWindow *mainWindow, const SceneManagerPtr &sceneMgr)
		: Super(app, "gamemode"), _sceneMgr(sceneMgr), _mainWindow(mainWindow) {
	}
	virtual ~GameModePanel() = default;

	void init();
	void update(const char *id, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
