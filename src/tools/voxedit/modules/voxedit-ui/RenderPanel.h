/**
 * @file
 */

#pragma once

#include "core/SharedProgress.h"
#include "core/Var.h"
#include "core/concurrent/Future.h"
#include "ui/Panel.h"
#include "video/Camera.h"
#include "voxelpathtracer/PathTracer.h"
#include "voxelpathtracer/PathTracerScene.h"
#include <stdint.h>

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
	SceneManagerPtr _sceneMgr;
	video::Camera _camera;
	int _currentSample = 0;
	bool _sceneReady = false;
	bool _hovered = false;
	uint64_t _sceneStamp = 0;
	uint64_t _prepStamp = 0;
	core::Future<voxelpathtracer::PreparedGpuScene> _prepFuture;
	core::SharedProgress _prepProgress;
	core::VarPtr _sunAngle;

	void applySceneSun();
	void renderSettings(const scenegraph::SceneGraph &sceneGraph);
	void renderMenuBar(const scenegraph::SceneGraph &sceneGraph);
	void renderCameraManipulator(video::Camera &camera, float headerSize, bool sceneMode);
	void updateCameraInput(float headerSize);
	void ensureTraceCamera(const scenegraph::SceneGraph &sceneGraph);
	void renderPrepProgress();
	void cancelScenePrep();
	bool pollScenePrep(const scenegraph::SceneGraph &sceneGraph);
	void startScenePrep(const scenegraph::SceneGraph &sceneGraph, uint64_t stamp);
	void updateSceneState(const scenegraph::SceneGraph &sceneGraph);

public:
	RenderPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "render"), _sceneMgr(sceneMgr) {
	}
	void update(const char *id, const scenegraph::SceneGraph &sceneGraph);
	bool init();
	void shutdown();
	bool isHovered() const;
	bool isPreparing() const;
	float prepProgress() const;
	video::Camera &camera();
	const video::Camera &camera() const;
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

inline bool RenderPanel::isHovered() const {
	return _hovered;
}

inline bool RenderPanel::isPreparing() const {
	return _prepFuture.valid();
}

inline float RenderPanel::prepProgress() const {
	return _prepProgress.progress();
}

inline video::Camera &RenderPanel::camera() {
	return _camera;
}

inline const video::Camera &RenderPanel::camera() const {
	return _camera;
}

} // namespace voxedit
