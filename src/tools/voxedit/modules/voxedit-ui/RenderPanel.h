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

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class RenderPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	voxelpathtracer::PathTracer _pathTracer;
	video::TexturePtr _texture;
	image::ImagePtr _image;
	SceneManagerPtr _sceneMgr;
	int _currentSample = 0;

public:
	RenderPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "render"), _sceneMgr(sceneMgr) {
	}
	void update(const char *title, const scenegraph::SceneGraph &sceneGraph);
	bool init();
	void shutdown();
};

} // namespace voxedit
