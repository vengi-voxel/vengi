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
protected:
	using Super = video::WindowedApp;
	video::Camera _camera;
	frontend::Axis _axis;
	uint8_t _moveMask = 0;

	virtual void doRender() = 0;

public:
	TestApp(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	virtual ~TestApp();

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
	virtual void onWindowResize() override;
};
