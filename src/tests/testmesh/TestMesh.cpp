#include "testcore/TestMeshApp.h"

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	TestMeshApp app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
