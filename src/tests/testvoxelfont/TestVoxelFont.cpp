#include "TestVoxelFont.h"

TestVoxelFont::TestVoxelFont(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
		Super(filesystem, eventBus) {
}

core::AppState TestVoxelFont::onInit() {
	core::AppState state = Super::onInit();
	if (!_voxelFont.init("font.ttf", 14, " Helowrd!")) {
		Log::error("Failed to start voxel font test application - could not load the given font file");
		return core::AppState::Cleanup;
	}
	return state;
}

core::AppState TestVoxelFont::onCleanup() {
	core::AppState state = Super::onCleanup();
	return state;
}

void TestVoxelFont::doRender() {
	voxel::Mesh target(1000, 1000);
	_voxelFont.render("Hello world!", target);
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	TestVoxelFont app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
