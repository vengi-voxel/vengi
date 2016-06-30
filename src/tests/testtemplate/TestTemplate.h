/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"

class TestTemplate: public TestApp {
private:
	using Super = TestApp;

	void doRender() override;
public:
	TestTemplate(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
};
