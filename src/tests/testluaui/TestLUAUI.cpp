#include "TestLUAUI.h"
#include "io/Filesystem.h"
#include "nuklear/Nuklear.h"

TestLUAUI::TestLUAUI(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testluaui");
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestLUAUI app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
