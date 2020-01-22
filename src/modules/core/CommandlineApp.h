/**
 * @file
 */

#pragma once

#include "core/App.h"

namespace core {

class CommandlineApp : public core::App {
private:
	using Super = core::App;
public:
	CommandlineApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~CommandlineApp();

	virtual core::AppState onConstruct() override;
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
