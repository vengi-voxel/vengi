#include "TestTemplate.h"

TestTemplate::TestTemplate(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
}

core::AppState TestTemplate::onInit() {
	core::AppState state = Super::onInit();
	return state;
}

core::AppState TestTemplate::onCleanup() {
	core::AppState state = Super::onCleanup();
	return state;
}

void TestTemplate::doRender() {
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	TestTemplate app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
