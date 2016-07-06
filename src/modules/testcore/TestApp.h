/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Mesh.h"
#include "video/Camera.h"
#include "frontend/Axis.h"
#include "core/AppModule.h"

class TestApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
protected:
	video::Camera _camera;
	frontend::Axis _axis;
	core::VarPtr _rotationSpeed;
	uint8_t _moveMask = 0;
	bool _cameraMotion = false;

	virtual void doRender() = 0;

	inline void setCameraMotion(bool cameraMotion) {
		_cameraMotion = cameraMotion;
	}

public:
	TestApp(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	virtual ~TestApp();

	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onWindowResize() override;
};
