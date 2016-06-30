#include "TestTemplate.h"

TestTemplate::TestTemplate(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
}

void TestTemplate::doRender() {
}

int main(int argc, char *argv[]) {
	return core::getApp<TestTemplate>()->startMainLoop(argc, argv);
}
