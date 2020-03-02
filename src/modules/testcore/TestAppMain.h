/**
 * @file
 */

#pragma once
#include <SDL_main.h>
#include <memory>
#include "core/io/Filesystem.h"
#include "core/metric/Metric.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"

#define TEST_APP(testClassName) \
int main(int argc, char *argv[]) { \
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>(); \
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>(); \
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>(); \
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>(); \
	testClassName app(metric, filesystem, eventBus, timeProvider); \
	return app.startMainLoop(argc, argv); \
}
