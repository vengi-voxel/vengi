/**
 * @file
 */

#pragma once

#include "App.h"

namespace core {

class ConsoleApp : public App {
private:
	using Super = core::App;
public:
	ConsoleApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~ConsoleApp();

	virtual AppState onConstruct() override;
};

}
