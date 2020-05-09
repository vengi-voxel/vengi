/**
 * @file
 */
#include "TestTemplate.h"
#include "testcore/TestAppMain.h"

TestTemplate::TestTemplate(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testtemplate");
}

core::AppState TestTemplate::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	return state;
}

core::AppState TestTemplate::onCleanup() {
	// your cleanup here
	return Super::onCleanup();
}

void TestTemplate::doRender() {
}

TEST_APP(TestTemplate)
