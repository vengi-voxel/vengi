/**
 * @file
 */

#include "UITool.h"
#include "ui/Window.h"
#include "core/AppModule.h"
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
	if (!tb::tb_core_init(&_renderer)) {
		Log::error("failed to initialize the ui");
		return core::AppState::Cleanup;
	}
	if (!tb::g_tb_lng->Load("ui/lang/en.tb.txt")) {
		Log::warn("could not load the translation");
	}
	if (!tb::g_tb_skin->Load("ui/skin/skin.tb.txt", nullptr)) {
		Log::error("could not load the skin");
		return core::AppState::Cleanup;
	}
	tb::TBWidgetsAnimationManager::Init();
	ui::initFonts();
	return state;
}

core::AppState UITool::onRunning() {
	if (_argc != 2) {
		_exitCode = 1;
		Log::error("Usage: %s <inputfile>", _argv[0]);
		return core::AppState::Cleanup;
	}

	ui::Window window((ui::Window*) nullptr);
	if (!window.loadResourceFile(_argv[1])) {
		_exitCode = 1;
		Log::error("Failed to parse ui file '%s'", _argv[1]);
	}

	return core::AppState::Cleanup;
}

core::AppState UITool::onCleanup() {
	tb::TBAnimationManager::AbortAllAnimations();

	tb::TBWidgetsAnimationManager::Shutdown();
	tb::tb_core_shutdown();

	return core::App::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<UITool>()->startMainLoop(argc, argv);
}
