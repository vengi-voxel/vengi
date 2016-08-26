#include "TestTemplate.h"

TestTemplate::TestTemplate(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
}

void TestTemplate::doRender() {
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	TestTemplate app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);

}
