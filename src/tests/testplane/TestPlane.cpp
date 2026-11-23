/**
 * @file
 */
#include "TestPlane.h"
#include "testcore/TestAppMain.h"
#include "math/Vec3f.h"

TestPlane::TestPlane(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	setCameraMotion(true);
	setRenderAxis(true);
}

app::AppState TestPlane::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	if (!_shapeRenderer.init()) {
		return app::AppState::InitFailure;
	}
	_plane.set(math::Vec3f(1.0f, 0.5f, 0.5f), math::Vec3f(10.0, 10.0, 10.0f));
	_shapeBuilder.plane(_plane, true);
	_shapeRenderer.create(_shapeBuilder);

	return state;
}

void TestPlane::doRender() {
	_shapeRenderer.renderAll(camera());
}

app::AppState TestPlane::onCleanup() {
	_shapeBuilder.shutdown();
	_shapeRenderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestPlane)
