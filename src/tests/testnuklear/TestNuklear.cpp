#include "TestNuklear.h"
#include "io/Filesystem.h"
#include "nuklear/Nuklear.h"
#include "overview.c"

TestNuklear::TestNuklear(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testnuklear");
}

void TestNuklear::onRenderUI() {
	overview(&_ctx);
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	TestNuklear app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
