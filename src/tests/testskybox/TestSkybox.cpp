/**
 * @file
 */

#include "TestSkybox.h"
#include "testcore/TestAppMain.h"

TestSkybox::TestSkybox(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testskybox");
}

app::AppState TestSkybox::onConstruct() {
	app::AppState state = Super::onConstruct();
	_skyboxVar = core::Var::get("skybox", "bluecloud");
	return state;
}

app::AppState TestSkybox::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	camera().setPosition(glm::backward);
	camera().lookAt(glm::forward);
	if (!_skybox.init(_skyboxVar->strVal().c_str())) {
		Log::error("Failed to initialize the skybox");
		return app::AppState::InitFailure;
	}

	return state;
}

app::AppState TestSkybox::onCleanup() {
	app::AppState state = Super::onCleanup();
	_skybox.shutdown();
	return state;
}

void TestSkybox::doRender() {
	if (_skyboxVar->isDirty()) {
		_skybox.shutdown();
		_skybox.init(_skyboxVar->strVal().c_str());
		_skyboxVar->markClean();
	}
	_skybox.render(camera());
}

TEST_APP(TestSkybox)
#include "io/Filesystem.h"