/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace video {
class Camera;
}
namespace voxelformat {
class SceneGraph;
}

namespace voxedit {

struct LayerSettings;

class SceneGraphPanel {
private:
	bool _showNodeDetails = true;
	void newLayerButton(const voxelformat::SceneGraph &sceneGraph, LayerSettings* layerSettings);

public:
	bool _popupNewLayer = false;
	void update(video::Camera &camera, const char *title, LayerSettings* layerSettings, command::CommandExecutionListener &listener);
};

}
