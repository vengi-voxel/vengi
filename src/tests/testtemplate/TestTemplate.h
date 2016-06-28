/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"

class TestTemplate: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
public:
	TestTemplate(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~TestTemplate();

	core::AppState onInit() override;
	core::AppState onRunning() override;
};
