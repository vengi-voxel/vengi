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
	TestTemplate(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
};
