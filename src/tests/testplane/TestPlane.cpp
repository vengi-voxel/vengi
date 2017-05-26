#include "TestPlane.h"
#include "io/Filesystem.h"

TestPlane::TestPlane(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
	setCameraMotion(true);
	setRenderAxis(true);
}

core::AppState TestPlane::onInit() {
	core::AppState state = Super::onInit();
	if (!_shapeRenderer.init()) {
		return core::AppState::Cleanup;
	}
	_plane.set(glm::vec3(1.0f, 0.5f, 0.5f), glm::vec3(10.0, 10.0, 10.0f));
	_shapeBuilder.plane(_plane, true);
	_shapeRenderer.create(_shapeBuilder);

	return state;
}

void TestPlane::doRender() {
	_shapeRenderer.renderAll(_camera);
}

core::AppState TestPlane::onCleanup() {
	core::AppState state = Super::onCleanup();
	_shapeBuilder.shutdown();
	_shapeRenderer.shutdown();
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestPlane app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
