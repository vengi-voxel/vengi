/**
 * @file
 */

#include "TestNuklear.h"
#include "testcore/TestAppMain.h"
#include "core/io/Filesystem.h"
#include "ui/nuklear/Nuklear.h"
#include "overview.c"

TestNuklear::TestNuklear(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testnuklear");
}

bool TestNuklear::onRenderUI() {
	overview(&_ctx);
	return true;
}

TEST_APP(TestNuklear)
