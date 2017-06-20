/**
 * @file
 */

#include "AbstractBenchmark.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/command/Command.h"

namespace core {

void AbstractBenchmark::SetUp(benchmark::State& st) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	_benchmarkApp = new BenchmarkApp(filesystem, eventBus, timeProvider, this);
}

void AbstractBenchmark::TearDown(benchmark::State& st) {
	// prevent cvars from begin saved and reloaded for the next fiture in the test
	core::Var::shutdown();
	delete _benchmarkApp;
	_benchmarkApp = nullptr;
}

AbstractBenchmark::BenchmarkApp::BenchmarkApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractBenchmark* benchmark) :
		core::App(filesystem, eventBus, timeProvider, 10000), _benchmark(benchmark) {
	init(ORGANISATION, "benchmark");
	while (_curState < AppState::Running) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
}

AppState AbstractBenchmark::BenchmarkApp::onCleanup() {
	AppState state = core::App::onCleanup();
	_benchmark->onCleanupApp();
	return state;
}

AppState AbstractBenchmark::BenchmarkApp::onInit() {
	AppState state = core::App::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (hasArg("debug") || hasArg("-debug") || hasArg("--debug") || hasArg("-d")) {
		_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
		Log::init();
		Log::debug("Activate debug logging");
	} else {
		_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_WARN));
		Log::init();
	}

	if (!_benchmark->onInitApp()) {
		return AppState::Cleanup;
	}

	return state;
}

AbstractBenchmark::BenchmarkApp::~BenchmarkApp() {
	while (AppState::InvalidAppState != _curState) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
}

}
