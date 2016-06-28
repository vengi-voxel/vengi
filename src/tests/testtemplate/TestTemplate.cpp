#include "TestTemplate.h"

TestTemplate::TestTemplate(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		core::App(filesystem, eventBus, 21000) {
}

TestTemplate::~TestTemplate() {
}

core::AppState TestTemplate::onRunning() {
	return core::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	return core::getApp<TestTemplate>()->startMainLoop(argc, argv);
}
