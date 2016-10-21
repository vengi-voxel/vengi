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
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	_testApp = new TestApp(filesystem, eventBus, timeProvider, this);
	const bool isRunning = _testApp->_curState == AppState::Running;
	ASSERT_TRUE(isRunning) << "Failed to setup the test app properly";
}

void AbstractTest::TearDown() {
	core::Var::shutdown();
	delete _testApp;
}

AbstractTest::TestApp::TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractTest* test) :
		core::App(filesystem, eventBus, timeProvider, 10000), _test(test) {
	init("engine", "test");
	_argc = ::_argc;
	_argv = ::_argv;
	while (_curState < AppState::Running) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
}

AppState AbstractTest::TestApp::onCleanup() {
	AppState state = core::App::onCleanup();
	_test->onCleanupApp();
	return state;
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

	if (!_test->onInitApp()) {
		return AppState::Cleanup;
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
