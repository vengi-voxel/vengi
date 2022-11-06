/**
 * @file
 */

#pragma once
#include <SDL_main.h>
#include <memory>
#include "io/Filesystem.h"
#include "metric/Metric.h"
#include "core/TimeProvider.h"

#define TEST_APP(testClassName) \
int main(int argc, char *argv[]) { \
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>(); \
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>(); \
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>(); \
	testClassName app(metric, filesystem, timeProvider); \
	return app.startMainLoop(argc, argv); \
}
