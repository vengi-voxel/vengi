/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "compute/Compute.h"
#include "compute/Texture.h"
#include "voxel/polyvox/RawVolume.h"
#include "video/Buffer.h"
#include "TestgpumcShaders.h"

#define COMPUTEVIDEO
#include "TestgpumcComputeShaders.h"

#include <vector>

class TestGPUMC: public TestApp {
private:
	using Super = TestApp;

	bool _extractSurface = true;
	bool _writingTo3DTextures = false;

	int _totalSum = 0;

	compute::TexturePtr _rawData;
	std::vector<compute::TexturePtr> _images;

	std::vector<compute::Id> buffers;
	compute::Id cubeIndexesBuffer = compute::InvalidId;
	compute::TexturePtr cubeIndexesImage;

	video::Buffer _vbo;
	int32_t _vboIdx = -1;
	compute::Id _vboComputeBufferId = compute::InvalidId;

	compute::MarchingcubesShader& _computeShader;
	compute::MarchingcubesBufferShader& _computeShaderBuffer;
	std::shared_ptr<voxel::RawVolume> _volume;

	shader::VertexShader& _renderShader;

	int calculateTotalSum();
	void extractSurfaces();
	void doRender() override;
public:
	TestGPUMC(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
