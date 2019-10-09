/**
 * @file
 */

#include "UITool.h"
#include "ui/turbobadger/Window.h"
#include "ui/turbobadger/FontUtil.h"
#include "core/io/Filesystem.h"

UITool::UITool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	_initialLogLevel = SDL_LOG_PRIORITY_WARN;
	init(ORGANISATION, "uitool");
}

core::AppState UITool::onInit() {
	const core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (_argc != 2) {
		_exitCode = 1;
		Log::error("Usage: %s <inputfile>", _argv[0]);
		return core::AppState::InitFailure;
	}

	if (!tb::tb_core_init(&_renderer)) {
		Log::error("failed to initialize the ui");
		return core::AppState::InitFailure;
	}
	if (!tb::g_tb_lng->load("ui/lang/en.tb.txt")) {
		Log::warn("could not load the translation");
	}
	if (!tb::g_tb_skin->load("../shared/ui/skin/skin.tb.txt", nullptr)) {
		Log::error("could not load the skin from shared dir");
		return core::AppState::InitFailure;
	}
	ui::turbobadger::initFonts();

	_root.setRect(tb::TBRect(0, 0, 800, 600));
	_root.setSkinBg(TBIDC("background"));

	return state;
}

core::AppState UITool::onRunning() {
	ui::turbobadger::Window window((ui::turbobadger::Window*) nullptr);
	_root.addChild(&window);
	if (!window.loadResourceFile(_argv[1])) {
		_exitCode = 1;
		Log::error("Failed to parse ui file '%s'", _argv[1]);
	}
	_root.removeChild(&window);

	return core::AppState::Cleanup;
}

core::AppState UITool::onCleanup() {
	tb::TBAnimationManager::abortAllAnimations();

	tb::tb_core_shutdown();

	return Super::onCleanup();
}

CONSOLE_APP(UITool)
