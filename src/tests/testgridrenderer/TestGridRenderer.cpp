/**
 * @file
 */
#include "TestGridRenderer.h"
#include "color/Color.h"
#include "testcore/TestAppMain.h"

TestGridRenderer::TestGridRenderer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _gridRenderer(true, true, true), _aabb(glm::vec3(-32.0f), glm::vec3(32.0f)) {
	setCameraMotion(true);
	setRenderAxis(true);
	setRenderPlane(false);
}

app::AppState TestGridRenderer::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	if (!_gridRenderer.init()) {
		return app::AppState::InitFailure;
	}
	_gridRenderer.setGridResolution(1);
	_gridRenderer.update(_aabb);
	return state;
}

void TestGridRenderer::doRender() {
	_gridRenderer.render(camera(), _aabb);
}

app::AppState TestGridRenderer::onCleanup() {
	_gridRenderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestGridRenderer)
