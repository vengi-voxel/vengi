/**
 * @file
 */

#pragma once

#include "core/App.h"

class TestTemplate: public core::App {
public:
	TestTemplate(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~TestTemplate();

	core::AppState onRunning() override;
};
