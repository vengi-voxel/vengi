/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "video/Texture.h"
#include "voxelpathtracer/PathTracer.h"

namespace ui {
class IMGUIApp;
}

namespace voxedit {

class RenderPanel : public ui::Panel {
private:
	voxelpathtracer::PathTracer _pathTracer;
	video::TexturePtr _texture;
	image::ImagePtr _image;
	int _currentSample = 0;

public:
	PANEL_CLASS(RenderPanel)
	void update(const char *title, const scenegraph::SceneGraph &sceneGraph);
	bool init();
	void shutdown();
};

} // namespace voxedit
