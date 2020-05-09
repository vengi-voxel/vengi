/**
 * @file
 */
#include "TestPlane.h"
#include "testcore/TestAppMain.h"

TestPlane::TestPlane(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	setCameraMotion(true);
	setRenderAxis(true);
}

core::AppState TestPlane::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}
	if (!_shapeRenderer.init()) {
		return core::AppState::InitFailure;
	}
	_plane.set(glm::vec3(1.0f, 0.5f, 0.5f), glm::vec3(10.0, 10.0, 10.0f));
	_shapeBuilder.plane(_plane, true);
	_shapeRenderer.create(_shapeBuilder);

	return state;
}

void TestPlane::doRender() {
	_shapeRenderer.renderAll(camera());
}

core::AppState TestPlane::onCleanup() {
	_shapeBuilder.shutdown();
	_shapeRenderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestPlane)
