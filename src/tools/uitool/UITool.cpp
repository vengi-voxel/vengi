/**
 * @file
 */

#include "UITool.h"
#include "ui/Window.h"
#include "ui/FontUtil.h"

UITool::UITool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		core::App(filesystem, eventBus, 0) {
	init("engine", "uitool");
}

core::AppState UITool::onInit() {
	const core::AppState state = core::App::onInit();
	if (state == core::AppState::Cleanup) {
		return state;
	}

	if (_argc != 2) {
		_exitCode = 1;
		Log::error("Usage: %s <inputfile>", _argv[0]);
		return core::AppState::Cleanup;
	}

	if (!tb::tb_core_init(&_renderer)) {
		Log::error("failed to initialize the ui");
		return core::AppState::Cleanup;
	}
	if (!tb::g_tb_lng->Load("ui/lang/en.tb.txt")) {
		Log::warn("could not load the translation");
	}
	if (!tb::g_tb_skin->Load("../shared/ui/skin/skin.tb.txt", nullptr)) {
		Log::error("could not load the skin from shared dir");
		return core::AppState::Cleanup;
	}
	tb::TBWidgetsAnimationManager::Init();
	ui::initFonts();

	_root.SetRect(tb::TBRect(0, 0, 800, 600));
	_root.SetSkinBg(TBIDC("background"));

	return state;
}

core::AppState UITool::onRunning() {
	ui::Window window((ui::Window*) nullptr);
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

	return core::App::onCleanup();
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	UITool app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
