/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "ui/Panel.h"

namespace ui {
class IMGUIApp;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class ISceneRenderer;
typedef core::SharedPtr<ISceneRenderer> SceneRendererPtr;

class MainWindow;

class SceneDebugPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	SceneRendererPtr _sceneRenderer;
	MainWindow *_mainWindow;

public:
	SceneDebugPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const SceneRendererPtr &sceneRenderer, MainWindow *mainWindow);
	void update(const char *id);

#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override {
	}
#endif
};

} // namespace voxedit
