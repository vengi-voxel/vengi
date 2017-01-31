/**
 * @file
 */

#include "NoiseTool.h"
#include "io/Filesystem.h"
#include "ui/NoiseToolWindow.h"

NoiseTool::NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		ui::UIApp(filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "noisetool");
}

core::AppState NoiseTool::onInit() {
	core::AppState state = ui::UIApp::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	_window = new NoiseToolWindow(this);
	if (!_window->init()) {
		return core::AppState::Cleanup;
	}

	return state;
}

core::AppState NoiseTool::onRunning() {
	core::AppState state = ui::UIApp::onRunning();
	if (_window) {
		_window->update(_deltaFrame);
	}
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	NoiseTool app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
