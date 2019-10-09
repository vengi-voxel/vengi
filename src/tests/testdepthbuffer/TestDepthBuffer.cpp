/**
 * @file
 */

#include "TestDepthBuffer.h"
#include "core/io/Filesystem.h"

TestDepthBuffer::TestDepthBuffer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super("testdepthbuffer", metric, filesystem, eventBus, timeProvider) {
}

void TestDepthBuffer::doRender() {
	Super::doRender();
	_shadow.renderShadowMap(_camera);
}

TEST_APP(TestDepthBuffer)
