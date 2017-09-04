/**
 * @file
 */

#include "ConsoleApp.h"

namespace core {

ConsoleApp::ConsoleApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport, size_t threadPoolSize) :
		Super(filesystem, eventBus, timeProvider, traceport, threadPoolSize) {
}

ConsoleApp::~ConsoleApp() {
}

AppState ConsoleApp::onConstruct() {
	const core::AppState state = Super::onConstruct();

	registerArg("--verbose").setShort("-v").setDescription("Change log level to trace");
	const bool isVerbose = hasArg("--verbose");
	if (isVerbose) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal(SDL_LOG_PRIORITY_VERBOSE);
		Log::init();
	}
	return state;
}

}
