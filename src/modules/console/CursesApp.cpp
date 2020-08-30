/**
 * @file
 */

#include "CursesApp.h"

namespace console {

CursesApp::CursesApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		Super(metric, filesystem, eventBus, timeProvider, threadPoolSize) {
}

CursesApp::~CursesApp() {
}

app::AppState CursesApp::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_console.construct();
	return state;
}

app::AppState CursesApp::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	if (!_console.init()) {
		return app::AppState::InitFailure;
	}
	return state;
}

app::AppState CursesApp::onCleanup() {
	_console.shutdown();
	return Super::onCleanup();
}

app::AppState CursesApp::onRunning() {
	const app::AppState state = Super::onRunning();
	_console.update(_deltaFrameSeconds);
	return state;
}

}
