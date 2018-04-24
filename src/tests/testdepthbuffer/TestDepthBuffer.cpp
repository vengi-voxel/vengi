#include "TestDepthBuffer.h"
#include "video/ScopedViewPort.h"
#include "io/Filesystem.h"

TestDepthBuffer::TestDepthBuffer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testdepthbuffer");
}

void TestDepthBuffer::doRender() {
	Super::doRender();
	_shadow.renderShadowMap(_camera);
}

TEST_APP(TestDepthBuffer)
