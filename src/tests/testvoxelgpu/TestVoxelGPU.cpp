/**
 * @file
 */
#include "TestVoxelGPU.h"
#include "compute/Compute.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelgenerator/NoiseGenerator.h"
#include "testcore/TestAppMain.h"
#include <memory>

TestVoxelGPU::TestVoxelGPU(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _mesher(compute::MesherShader::getInstance()) {
	init(ORGANISATION, "testvoxelgpu");
}

app::AppState TestVoxelGPU::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!compute::init()) {
		return app::AppState::InitFailure;
	}

	if (!_mesher.setup()) {
		return app::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return app::AppState::InitFailure;
	}

	voxel::Region region(0, 0, 0, _workSize.x - 1, _workSize.y - 1, _workSize.z - 1);
	_volume = std::make_shared<voxel::RawVolume>(region);
	math::Random random;
	voxel::RawVolumeWrapper wrapper(_volume.get());
	voxelgenerator::noise::generate(wrapper, 4, 2.0f, 0.01f, 0.5f, voxelgenerator::noise::NoiseType::ridgedMF, random);

	compute::TextureConfig cfg3d;
	cfg3d.type(compute::TextureType::Texture3D).format(compute::TextureFormat::RG).dataformat(compute::TextureDataFormat::UNSIGNED_INT8);
	static_assert(sizeof(voxel::Voxel) == 2, "Texture type must be changed if the voxel size is not 16 bits anymore");
	_volumeTexture = core::make_shared<compute::Texture>(cfg3d, _workSize, "volume");
	if (!_volumeTexture->upload(_volume->data())) {
		Log::error("Failed to upload volume data");
	}

	_output.resize(_workSize.x * _workSize.y * 4);
	_output.shrink_to_fit();

	video::clearColor(::core::Color::White);

	return state;
}

app::AppState TestVoxelGPU::onCleanup() {
	_mesher.shutdown();
	_volumeTexture->shutdown();
	_volume = std::shared_ptr<voxel::RawVolume>();
	return Super::onCleanup();
}

app::AppState TestVoxelGPU::onRunning() {
	if (!_mesher.extractCubicMesh(*_volumeTexture.get(), _output, _workSize.x, _workSize.y, _workSize.z, _workSize)) {
		Log::error("Failed to execute the compute kernel");
		return app::AppState::Cleanup;
	}
	return Super::onRunning();
}

void TestVoxelGPU::onRenderUI() {
	Super::onRenderUI();
}

void TestVoxelGPU::doRender() {
}

TEST_APP(TestVoxelGPU)
