/**
 * @file
 */
#include "TestComputeShader.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedState.h"
#include "video/ScopedViewPort.h"
#include "color/Color.h"
#include "core/Log.h"
#include <glm/vec3.hpp>

TestComputeShader::TestComputeShader(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "testcomputeshader");
	setCameraMotion(false);
	setRenderAxis(false);
	setRenderPlane(false);
	_allowRelativeMouseMode = false;
}

app::AppState TestComputeShader::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!video::hasFeature(video::Feature::ComputeShaders)) {
		Log::error("This test needs compute shader support");
		return app::AppState::InitFailure;
	}

	setUICamera();

	if (!_renderer.init()) {
		Log::error("Failed to init the texture renderer");
		return app::AppState::InitFailure;
	}

	if (!_computeShader.setup()) {
		Log::error("Failed to init the compute shader");
		return app::AppState::InitFailure;
	}

	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA32F);
	cfg.filter(video::TextureFilter::Nearest);
	_texture = video::createTexture(cfg, TextureSize, TextureSize, appname());
	if (!_texture) {
		Log::error("Failed to create compute output texture");
		return app::AppState::InitFailure;
	}

	video::bindImage(_texture->handle(), video::AccessMode::Write, _computeShader.getImageFormatImgOutput());
	video::clearColor(color::Black());
	return state;
}

app::AppState TestComputeShader::onCleanup() {
	_computeShader.shutdown();
	_renderer.shutdown();
	if (_texture) {
		_texture->shutdown();
	}
	return Super::onCleanup();
}

void TestComputeShader::doRender() {
	_computeShader.activate();
	const uint32_t groups = (uint32_t)((TextureSize + 7) / 8);
	_computeShader.run(glm::uvec3(groups, groups, 1u), video::MemoryBarrierType::ShaderImageAccess);
	_computeShader.deactivate();

	video::ScopedState depthTest(video::State::DepthTest, false);
	video::ScopedTexture texture(_texture, video::TextureUnit::Zero);
	video::ScopedViewPort viewPort(0, 0, frameBufferDimension().x, frameBufferDimension().y);
	_renderer.render();
}

TEST_APP(TestComputeShader)
