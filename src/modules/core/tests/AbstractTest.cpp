/**
 * @file
 */

#include "AbstractTest.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/command/Command.h"

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
	// prevent cvars from begin saved and reloaded for the next fixture in the test
	core::Var::shutdown();
	delete _testApp;
	_testApp = nullptr;
}

AbstractTest::TestApp::TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractTest* test) :
		Super(filesystem, eventBus, timeProvider, 10000), _test(test) {
	init(ORGANISATION, "test");
	_argc = ::_argc;
	_argv = ::_argv;
	while (_curState < AppState::Running) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
}

AppState AbstractTest::TestApp::onCleanup() {
	AppState state = Super::onCleanup();
	_test->onCleanupApp();
	return state;
}

AppState AbstractTest::TestApp::onInit() {
	AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!_test->onInitApp()) {
		return AppState::InitFailure;
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
