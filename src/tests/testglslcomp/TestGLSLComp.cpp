/**
 * @file
 */
#include "TestGLSLComp.h"
#include "testcore/TestAppMain.h"
#include "core/Color.h"
#include "video/Camera.h"
#include "video/ScopedViewPort.h"

TestGLSLComp::TestGLSLComp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testglslcomp");
}

core::AppState TestGLSLComp::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!video::hasFeature(video::Feature::ComputeShaders)) {
		Log::error("This test needs compute shader support");
		return core::AppState::InitFailure;
	}

	_camera = video::uiCamera(glm::ivec2(0), frameBufferDimension(), windowDimension());

	if (!_renderer.init(frameBufferDimension())) {
		Log::error("Failed to init the texture renderer");
		return core::AppState::InitFailure;
	}

	if (!_testShader.setup()) {
		Log::error("Failed to init the compute shader");
		return core::AppState::InitFailure;
	}

	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA32F);
	_texture = video::createTexture(cfg, 512, 512, appname());
	_texture->upload(nullptr);
	video::bindImage(_texture->handle(), video::AccessMode::Write, _testShader.getImageFormatImgOutput());

	video::clearColor(::core::Color::White);
	return state;
}

core::AppState TestGLSLComp::onCleanup() {
	core::AppState state = Super::onCleanup();
	_testShader.shutdown();
	_renderer.shutdown();
	if (_texture) {
		_texture->shutdown();
	}
	return state;
}

void TestGLSLComp::doRender() {
	_testShader.activate();
	_testShader.run(glm::uvec3(_texture->width(), _texture->height(), 1), true);

	video::ScopedTexture texture(_texture, video::TextureUnit::Zero);
	video::ScopedViewPort viewPort(0, 0, frameBufferDimension().x, frameBufferDimension().y);
	_renderer.render(_camera.projectionMatrix());
}

TEST_APP(TestGLSLComp)
