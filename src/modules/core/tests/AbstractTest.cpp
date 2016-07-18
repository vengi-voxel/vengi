/**
 * @file
 */

#include "AbstractTest.h"
#include "core/Log.h"
#include "core/Var.h"

extern char **_argv;
extern int _argc;

namespace core {

void AbstractTest::SetUp() {
	const core::EventBusPtr eventBus(new core::EventBus());
	const io::FilesystemPtr filesystem(new io::Filesystem());
	_testApp = new TestApp(filesystem, eventBus);
}

void AbstractTest::TearDown() {
	delete _testApp;
	Var::shutdown();
}

AbstractTest::TestApp::TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
		core::App(filesystem, eventBus, 10000) {
	init("engine", "test");
	_argc = ::_argc;
	_argv = ::_argv;
	while (_curState < AppState::Running) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
}

AppState AbstractTest::TestApp::onInit() {
	AppState state = core::App::onInit();
	if (hasArg("debug")) {
		_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
		Log::init();
		Log::debug("Activate debug logging");
	} else {
		_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_WARN));
		Log::init();
	}
	return state;
}

AbstractTest::TestApp::~TestApp() {
	while (AppState::InvalidAppState != _curState) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
}

}
