/**
 * @file
 */

#include "UITool.h"
#include "ui/turbobadger/Window.h"
#include "ui/turbobadger/FontUtil.h"
#include "io/Filesystem.h"

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
	if (!tb::g_tb_lng->Load("ui/lang/en.tb.txt")) {
		Log::warn("could not load the translation");
	}
	if (!tb::g_tb_skin->Load("../shared/ui/skin/skin.tb.txt", nullptr)) {
		Log::error("could not load the skin from shared dir");
		return core::AppState::InitFailure;
	}
	tb::TBWidgetsAnimationManager::Init();
	ui::turbobadger::initFonts();

	_root.SetRect(tb::TBRect(0, 0, 800, 600));
	_root.SetSkinBg(TBIDC("background"));

	return state;
}

core::AppState UITool::onRunning() {
	ui::turbobadger::Window window((ui::turbobadger::Window*) nullptr);
	_root.AddChild(&window);
	if (!window.loadResourceFile(_argv[1])) {
		_exitCode = 1;
		Log::error("Failed to parse ui file '%s'", _argv[1]);
	}
	_root.RemoveChild(&window);

	return core::AppState::Cleanup;
}

core::AppState UITool::onCleanup() {
	tb::TBAnimationManager::AbortAllAnimations();

	tb::TBWidgetsAnimationManager::Shutdown();
	tb::tb_core_shutdown();

	return Super::onCleanup();
}

CONSOLE_APP(UITool)
