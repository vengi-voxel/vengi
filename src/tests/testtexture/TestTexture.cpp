/**
 * @file
 */
#include "TestTexture.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedViewPort.h"
#include "core/Log.h"
#include "image/Image.h"

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

	_reupload = image::loadImage("texture.png");
	_texture = video::createTextureFromImage(_reupload);
	if (!_texture) {
		Log::error("Failed to load texture");
		return app::AppState::InitFailure;
	}

	video::clearColor(::color::White());

	return state;
}

void TestTexture::doRender() {
	video::ScopedViewPort viewPort(0, 0, frameBufferDimension().x, frameBufferDimension().y);
	video::ScopedTexture texture(_texture, video::TextureUnit::Zero);
	_renderer.render();
	if (_reupload) {
		// Replace a texture already referenced by a draw in this frame.
		_texture->upload(_reupload);
		_reupload = {};
	}
}

app::AppState TestTexture::onCleanup() {
	if (_texture) {
		_texture->shutdown();
	}
	_renderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestTexture)
