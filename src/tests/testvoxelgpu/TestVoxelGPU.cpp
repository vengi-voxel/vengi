/**
 * @file
 */
#include "TestVoxelGPU.h"
#include "io/Filesystem.h"
#include "compute/Compute.h"
#include "voxel/MaterialColor.h"

TestVoxelGPU::TestVoxelGPU(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _mesher(compute::MesherShader::getInstance()) {
	init(ORGANISATION, "testvoxelgpu");
}

core::AppState TestVoxelGPU::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!compute::init()) {
		return core::AppState::InitFailure;
	}

	if (!_mesher.setup()) {
		return core::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	video::clearColor(::core::Color::White);

	return state;
}

core::AppState TestVoxelGPU::onCleanup() {
	core::AppState state = Super::onCleanup();
	_mesher.shutdown();
	return state;
}

void TestVoxelGPU::doRender() {
}

TEST_APP(TestVoxelGPU)
