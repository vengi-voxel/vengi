/**
 * @file
 */
#include "TestVoxelGPU.h"
#include "io/Filesystem.h"

TestVoxelGPU::TestVoxelGPU(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testvoxelgpu");
}

core::AppState TestVoxelGPU::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	return state;
}

core::AppState TestVoxelGPU::onCleanup() {
	core::AppState state = Super::onCleanup();
	return state;
}

void TestVoxelGPU::doRender() {
}

TEST_APP(TestVoxelGPU)
