/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Mesh.h"
#include "video/Camera.h"
#include "frontend/Axis.h"
#include "frontend/Plane.h"

class TestApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
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
	TestApp(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	virtual ~TestApp();

	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onMouseWheel(int32_t x, int32_t y) override;
	virtual void onWindowResize() override;
};
