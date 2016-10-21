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
	TestTemplate(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
