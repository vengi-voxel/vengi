/**
 * @file
 */

#pragma once

#include "testcore/TestMeshApp.h"

class TestDepthBuffer: public TestMeshApp {
private:
	using Super = TestMeshApp;

	void doRender() override;
public:
	TestDepthBuffer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);
};
