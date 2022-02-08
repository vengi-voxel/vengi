/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace video {
class Camera;
}

namespace voxedit {

class SceneGraphPanel {
public:
	void update(video::Camera& camera, const char *title, command::CommandExecutionListener &listener);
};

}
