/**
 * @file
 */
#include "TestTexture.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedViewPort.h"
#include "color/Color.h"
#include "core/Log.h"

TestTexture::TestTexture(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "testtexture");
}

app::AppState TestTexture::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	setUICamera();

	if (!_renderer.init()) {
		Log::error("Failed to init the texture renderer");
		return app::AppState::InitFailure;
	}

	_texture = video::createTextureFromImage("texture.png");
	if (!_texture) {
		Log::error("Failed to load texture");
		return app::AppState::InitFailure;
	}

	video::clearColor(::core::Color::White());

	return state;
}

void TestTexture::doRender() {
	video::ScopedViewPort viewPort(0, 0, frameBufferDimension().x, frameBufferDimension().y);
	video::ScopedTexture texture(_texture, video::TextureUnit::Zero);
	_renderer.render();
}

app::AppState TestTexture::onCleanup() {
	if (_texture) {
		_texture->shutdown();
	}
	_renderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestTexture)
