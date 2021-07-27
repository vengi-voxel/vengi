/**
 * @file
 */

#include "TextConsoleApp.h"

namespace console {

TextConsoleApp::TextConsoleApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		Super(metric, filesystem, eventBus, timeProvider, threadPoolSize) {
}

TextConsoleApp::~TextConsoleApp() {
}

app::AppState TextConsoleApp::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_console.construct();
	return state;
}

app::AppState TextConsoleApp::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	if (!_console.init()) {
		return app::AppState::InitFailure;
	}
	return state;
}

app::AppState TextConsoleApp::onCleanup() {
	_console.shutdown();
	return Super::onCleanup();
}

app::AppState TextConsoleApp::onRunning() {
	const app::AppState state = Super::onRunning();
	_console.update(_deltaFrameSeconds);
	return state;
}

}
