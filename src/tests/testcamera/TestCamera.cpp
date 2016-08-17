#include "TestCamera.h"

// TODO: zooming should update the far and near plane of the render camera (maybe alt + ctrl pressed)
// TODO: onMouseMotion for renderCamera (maybe also while ctrl or alt is held)
TestCamera::TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
	setRenderPlane(false);
	setRenderAxis(true);
}

core::AppState TestCamera::onInit() {
	core::AppState state = Super::onInit();
	_renderCamera[0].update(0l);
	_renderCamera[1].update(0l);

	if (!_frustums[0].init(_renderCamera[0], core::Color::Red)) {
		return core::AppState::Cleanup;
	}
	if (!_frustums[1].init(_renderCamera[1], core::Color::Yellow)) {
		return core::AppState::Cleanup;
	}

	static_assert(CAMERAS == 2, "Unexpected amount of cameras");

	for (int i = 0; i < CAMERAS; ++i) {
		const float p = i * 10.0f + 1.0f;
		_renderCamera[i].init(dimension());
		_renderCamera[i].setAspectRatio(_aspect);
		_renderCamera[i].setRotationType(video::CameraRotationType::Target);
		_renderCamera[i].setOmega(glm::vec3(0.0f, 0.001f, 0.0f));
		// TODO: per camera settings
		_renderCamera[i].setPosition(glm::vec3(p, 10.0f, p));
		_renderCamera[i].setTarget(glm::vec3(10.0f, 70.0f, 10.0f));
		_renderCamera[i].setNearPlane(5.0f);
		_renderCamera[i].setFarPlane(40.0f);
	}

	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setTarget(_renderCamera[0].position());

	return state;
}

core::AppState TestCamera::onRunning() {
	core::AppState state = Super::onRunning();
	for (int i = 0; i < CAMERAS; ++i) {
		_renderCamera[i].update(_deltaFrame);
	}
	_camera.setTarget(_renderCamera[_targetCamera].position());
	return state;
}

core::AppState TestCamera::onCleanup() {
	core::AppState state = Super::onCleanup();
	for (int i = 0; i < CAMERAS; ++i) {
		_frustums[i].shutdown();
	}
	return state;
}

void TestCamera::doRender() {
	for (int i = 0; i < CAMERAS; ++i) {
		_frustums[i].render(_camera, _renderCamera[i]);
	}
}

int main(int argc, char *argv[]) {
	return core::getApp<TestCamera>()->startMainLoop(argc, argv);
}
