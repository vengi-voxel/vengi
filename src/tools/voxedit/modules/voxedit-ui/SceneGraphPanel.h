/**
 * @file
 */

#pragma once

namespace video {
class Camera;
}

namespace voxedit {

class SceneGraphPanel {
public:
	void update(video::Camera& camera, const char *title);
};

}
