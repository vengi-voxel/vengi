/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "metric/Metric.h"

namespace app {

/**
 * Base application class that handles command line arguments, but doesn't support console input
 */
class CommandlineApp : public app::App {
private:
	using Super = app::App;
public:
	CommandlineApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~CommandlineApp();

	virtual app::AppState onConstruct() override;
};

}

#define CONSOLE_APP(consoleAppName) \
int main(int argc, char *argv[]) { \
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>(); \
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>(); \
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>(); \
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>(); \
	consoleAppName app(metric, filesystem, eventBus, timeProvider); \
	return app.startMainLoop(argc, argv); \
}
