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

core::AppState CursesApp::onConstruct() {
	const core::AppState state = Super::onConstruct();
	_console.construct();
	return state;
}

core::AppState CursesApp::onInit() {
	const core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}
	if (!_console.init()) {
		return core::AppState::InitFailure;
	}
	return state;
}

core::AppState CursesApp::onCleanup() {
	_console.shutdown();
	return Super::onCleanup();
}

core::AppState CursesApp::onRunning() {
	const core::AppState state = Super::onRunning();
	_console.update(_deltaFrameSeconds);
	return state;
}

}
