/**
 * @file
 */

#include "NoiseTool.h"
#include "ui/NoiseParametersWindow.h"

NoiseTool::NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		ui::UIApp(filesystem, eventBus, timeProvider) {
	init("engine", "noisetool");
}

core::AppState NoiseTool::onInit() {
	core::AppState state = ui::UIApp::onInit();
	if (state != core::Running)
		return state;

	new NoiseParametersWindow(this);

	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	NoiseTool app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
