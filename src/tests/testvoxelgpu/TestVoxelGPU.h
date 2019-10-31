/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "compute/Texture.h"
#include "voxel/RawVolume.h"
#include "TestvoxelgpuComputeShaders.h"
#include <memory>

class TestVoxelGPU: public TestApp {
private:
	using Super = TestApp;
	compute::MesherShader& _mesher;
	std::vector<uint8_t> _output;
	compute::TexturePtr _volumeTexture;
	std::shared_ptr<voxel::RawVolume> _volume;
	glm::ivec3 _workSize { 64, 64, 64 };

	void onRenderUI() override;
	void doRender() override;
public:
	TestVoxelGPU(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
	virtual core::AppState onRunning() override;
};
