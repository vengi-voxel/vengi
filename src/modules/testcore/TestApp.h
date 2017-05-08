/**
 * @file
 */

#pragma once

#include "imgui/IMGUIApp.h"
#include "video/Mesh.h"
#include "video/Camera.h"
#include "frontend/Axis.h"
#include "frontend/Plane.h"

class TestApp: public imgui::IMGUIApp {
private:
	using Super = imgui::IMGUIApp;
	bool _cameraMotion = false;
	bool _renderPlane = false;
	bool _renderAxis = true;
protected:
	video::Camera _camera;
	frontend::Axis _axis;
	frontend::Plane _plane;
	core::VarPtr _rotationSpeed;
	uint8_t _moveMask = 0;
	float _cameraSpeed = 0.1f;

	virtual void doRender() = 0;

	inline void setCameraSpeed(float cameraSpeed) {
		_cameraSpeed = cameraSpeed;
	}

	inline void setCameraMotion(bool cameraMotion) {
		_cameraMotion = cameraMotion;
		SDL_SetRelativeMouseMode(_cameraMotion ? SDL_TRUE : SDL_FALSE);
	}

	inline void setRenderPlane(bool renderPlane) {
		_renderPlane = renderPlane;
	}

	inline void setRenderAxis(bool renderAxis) {
		_renderAxis = renderAxis;
	}

public:
	TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
	virtual ~TestApp();

	video::Camera& camera();

	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual void beforeUI() override;
	virtual void onRenderUI() override;
	virtual core::AppState onCleanup() override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onWindowResize() override;
};

inline video::Camera& TestApp::camera() {
	return _camera;
}
