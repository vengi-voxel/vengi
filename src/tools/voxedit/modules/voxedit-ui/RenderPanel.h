/**
 * @file
 */

#pragma once

#include "video/Texture.h"
#include "voxelpathtracer/PathTracer.h"

namespace voxedit {

class RenderPanel {
private:
	voxelpathtracer::PathTracer _pathTracer;
	video::TexturePtr _texture;
	image::ImagePtr _image;
	int _currentSample = 0;

public:
	void update(const char *title, const scenegraph::SceneGraph &sceneGraph);
	bool init();
	void shutdown();
};

} // namespace voxedit
