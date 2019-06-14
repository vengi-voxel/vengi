/**
 * @file
 */
#include "TestTexture.h"
#include "core/Color.h"
#include "video/Camera.h"
#include "video/ScopedViewPort.h"
#include "io/Filesystem.h"

TestTexture::TestTexture(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testtexture");
}

core::AppState TestTexture::onInit() {
	const core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}
	_camera.setMode(video::CameraMode::Orthogonal);
	_camera.setNearPlane(-1.0f);
	_camera.setFarPlane(1.0f);

	if (!_renderer.init(pixelDimension())) {
		Log::error("Failed to init the texture renderer");
		return core::AppState::InitFailure;
	}

	_texture = video::createTextureFromImage("texture.png");
	if (!_texture) {
		Log::error("Failed to load texture");
		return core::AppState::InitFailure;
	}

	video::clearColor(::core::Color::White);

	return state;
}

void TestTexture::doRender() {
	video::ScopedViewPort viewPort(0, 0, pixelDimension().x, pixelDimension().y);
	video::ScopedTexture texture(_texture, video::TextureUnit::Zero);
	_renderer.render(_camera.projectionMatrix());
}

core::AppState TestTexture::onCleanup() {
	if (_texture) {
		_texture->shutdown();
	}
	_renderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestTexture)
