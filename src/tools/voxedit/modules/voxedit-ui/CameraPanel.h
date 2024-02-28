/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "ui/Panel.h"

namespace video {
class Camera;
}

namespace voxedit {

class CameraPanel : public ui::Panel {
private:
	using Super = ui::Panel;

public:
	CameraPanel(ui::IMGUIApp *app) : Super(app) {
	}
	void update(const char *title, video::Camera &camera, command::CommandExecutionListener &listener);
};

} // namespace voxedit
