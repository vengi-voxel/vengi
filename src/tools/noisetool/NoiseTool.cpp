/**
 * @file
 */

#include "NoiseTool.h"
#include "ui/NoiseParametersWindow.h"

NoiseTool::NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
		ui::UIApp(filesystem, eventBus) {
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
	NoiseTool app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
