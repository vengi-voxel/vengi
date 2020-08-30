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

core::AppState TestSkybox::onConstruct() {
	core::AppState state = Super::onConstruct();
	_skyboxVar = core::Var::get("skybox", "bluecloud");
	return state;
}

core::AppState TestSkybox::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	camera().setPosition(glm::backward);
	camera().lookAt(glm::forward);
	if (!_skybox.init(_skyboxVar->strVal().c_str())) {
		Log::error("Failed to initialize the skybox");
		return core::AppState::InitFailure;
	}

	return state;
}

core::AppState TestSkybox::onCleanup() {
	core::AppState state = Super::onCleanup();
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