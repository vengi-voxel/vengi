/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "ui/Panel.h"

namespace video {
class Camera;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class CameraPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;

public:
	CameraPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app), _sceneMgr(sceneMgr) {
	}
	void update(const char *title, video::Camera &camera, command::CommandExecutionListener &listener);
};

} // namespace voxedit
