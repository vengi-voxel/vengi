/**
 * @file
 */

#include "ConsoleApp.h"
#include "core/Var.h"

namespace core {

ConsoleApp::ConsoleApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		Super(metric, filesystem, eventBus, timeProvider, threadPoolSize) {
}

ConsoleApp::~ConsoleApp() {
}

AppState ConsoleApp::onConstruct() {
	const core::AppState state = Super::onConstruct();

	registerArg("--trace").setDescription("Change log level to trace");
	registerArg("--debug").setDescription("Change log level to debug");
	registerArg("--info").setDescription("Change log level to info");
	registerArg("--warn").setDescription("Change log level to warn");
	registerArg("--error").setDescription("Change log level to error");

	bool changed = false;
	if (hasArg("--trace")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal(SDL_LOG_PRIORITY_VERBOSE);
		changed = true;
	} else if (hasArg("--debug")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal(SDL_LOG_PRIORITY_DEBUG);
		changed = true;
	} else if (hasArg("--info")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal(SDL_LOG_PRIORITY_INFO);
		changed = true;
	} else if (hasArg("--warn")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal(SDL_LOG_PRIORITY_WARN);
		changed = true;
	} else if (hasArg("--error")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal(SDL_LOG_PRIORITY_ERROR);
		changed = true;
	}
	if (changed) {
		Log::init();
	}
	return state;
}

}
