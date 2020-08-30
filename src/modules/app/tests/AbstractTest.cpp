/**
 * @file
 */

#include "AbstractTest.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/command/Command.h"
#include "metric/Metric.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "core/io/Filesystem.h"

extern char **_argv;
extern int _argc;

namespace core {

core::String AbstractTest::fileToString(const core::String& filename) const {
	const io::FilesystemPtr& fs = _testApp->filesystem();
	const io::FilePtr& file = fs->open(filename);
	if (!file->exists()) {
		Log::error("Failed to load file '%s'", filename.c_str());
		return "";
	}
	return file->load();
}

void AbstractTest::SetUp() {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	_testApp = new TestApp(metric, filesystem, eventBus, timeProvider, this);
	const bool isRunning = _testApp->_curState == AppState::Running;
	ASSERT_TRUE(isRunning) << "Failed to setup the test app properly";
}

void AbstractTest::TearDown() {
	// prevent cvars from begin saved and reloaded for the next fixture in the test
	core::Var::shutdown();
	delete _testApp;
	_testApp = nullptr;
}

AbstractTest::TestApp::TestApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractTest* test) :
		Super(metric, filesystem, eventBus, timeProvider), _test(test) {
	init(ORGANISATION, "test");
	_initialLogLevel = SDL_LOG_PRIORITY_WARN;
	_argc = ::_argc;
	_argv = ::_argv;
	while (_curState < AppState::Running) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
}

AppState AbstractTest::TestApp::onCleanup() {
	_test->onCleanupApp();
	return Super::onCleanup();
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
