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

	void renderMenuBar(const scenegraph::SceneGraph &sceneGraph);

public:
	RenderPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "render"), _sceneMgr(sceneMgr) {
	}
	void updateSettings(const char *id, const scenegraph::SceneGraph &sceneGraph);
	void update(const char *id, const scenegraph::SceneGraph &sceneGraph);
	bool init();
	void shutdown();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
