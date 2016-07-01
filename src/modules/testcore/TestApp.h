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
	uint8_t _moveMask = 0;

	virtual void doRender() = 0;

public:
	TestApp(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	virtual ~TestApp();

	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
	virtual void onWindowResize() override;
};
