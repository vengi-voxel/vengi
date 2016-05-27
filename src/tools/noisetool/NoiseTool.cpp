/**
 * @file
 */

#include "NoiseTool.h"
#include "ui/NoiseParametersWindow.h"

#include "sauce/NoiseToolInjector.h"

// tool for testing the world createXXX functions without starting the application
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
	return getInjector()->get<NoiseTool>()->startMainLoop(argc, argv);
}
