/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/DepthBuffer.h"

class TestDepthBuffer: public video::WindowedApp {
private:
	video::DepthBuffer _depthBuffer;
public:
	TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~TestDepthBuffer();

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
