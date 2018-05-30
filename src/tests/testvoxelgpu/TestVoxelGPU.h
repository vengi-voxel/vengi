/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"

class TestVoxelGPU: public TestApp {
private:
	using Super = TestApp;

	void doRender() override;
public:
	TestVoxelGPU(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
