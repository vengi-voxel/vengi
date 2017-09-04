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
	ConsoleApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport, size_t threadPoolSize = 1);
	virtual ~ConsoleApp();

	virtual AppState onConstruct() override;
};

}
