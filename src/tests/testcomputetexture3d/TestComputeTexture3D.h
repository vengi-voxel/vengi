/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "compute/Texture.h"
#include "video/Texture.h"
#include "voxel/RawVolume.h"
#include "Testcomputetexture3dComputeShaders.h"
#include "render/TextureRenderer.h"
#include <memory>

class TestComputeTexture3D: public TestApp {
private:
	using Super = TestApp;
	video::TexturePtr _texture2D;
	render::TextureRenderer _renderer;
	compute::TexturePtr _texture3DCompute;
	compute::RenderShader& _renderShader;
	glm::ivec2 _workSize { 64, 64 };
	int _depth = 8;;
	float _slice = 0.0f;
	std::vector<uint8_t> _output;
	std::shared_ptr<voxel::RawVolume> _volume;

	core::AppState onRunning() override;
	void onRenderUI() override;
	void doRender() override;
	void initVolume();
public:
	TestComputeTexture3D(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
