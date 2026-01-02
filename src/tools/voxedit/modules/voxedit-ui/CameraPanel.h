/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "ui/Panel.h"
#include "voxelrender/RenderUtil.h"

namespace video {
class Camera;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

/**
 * @brief Get the current camera values and allows one to modify them or create a camera node from them.
 */
class CameraPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;

	void addToolbar(command::CommandExecutionListener &listener, video::Camera &camera);

public:
	CameraPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "camera"), _sceneMgr(sceneMgr) {
	}
	static void cameraProjectionCombo(video::Camera &camera);
	static void cameraModeCombo(command::CommandExecutionListener *listener, voxelrender::SceneCameraMode &camMode);
	static void cameraOptions(command::CommandExecutionListener *listener, video::Camera &camera,
							  voxelrender::SceneCameraMode camMode);

	void update(const char *id, video::Camera &camera, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
