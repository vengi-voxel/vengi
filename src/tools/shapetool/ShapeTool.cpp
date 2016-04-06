#include "ShapeTool.h"
#include "sauce/ShapeToolInjector.h"
#include "core/App.h"
#include "core/Process.h"
#include "video/Shader.h"

// tool for testing the world createXXX functions without starting the application
ShapeTool::ShapeTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		ui::UIApp(filesystem, eventBus) {
	init("engine", "shapetool");
}

ShapeTool::~ShapeTool() {
}

core::AppState ShapeTool::onRunning() {
	core::AppState state = UIApp::onRunning();


	return state;
}

int main(int argc, char *argv[]) {
	return getInjector()->get<ShapeTool>()->startMainLoop(argc, argv);
}
