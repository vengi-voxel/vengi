#include "TestTexture.h"

TestTexture::TestTexture(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
}

core::AppState TestTexture::onInit() {
	_camera.setOrtho(true);
	const core::AppState state = Super::onInit();

	if (!_textureShader.setup()) {
		Log::error("Failed to init the texture shader");
		return core::AppState::Cleanup;
	}

	_texture = video::createTextureFromImage("texture.png");
	if (!_texture) {
		Log::error("Failed to load texture");
		return core::AppState::Cleanup;
	}

	const glm::ivec2& fullscreenQuadIndices = _texturedFullscreenQuad.createFullscreenTexturedQuad();
	_texturedFullscreenQuad.addAttribute(_textureShader.getLocationPos(), fullscreenQuadIndices.x, 3);
	_texturedFullscreenQuad.addAttribute(_textureShader.getLocationTexcoord(), fullscreenQuadIndices.y, 2);

	return state;
}

void TestTexture::doRender() {
	video::ScopedShader scoped(_textureShader);
	_textureShader.setTexture(0);
	_texture->bind();
	core_assert_always(_texturedFullscreenQuad.bind());
	glDrawArrays(GL_TRIANGLES, 0, _texturedFullscreenQuad.elements(0));
	_texturedFullscreenQuad.unbind();
	_texture->unbind();
}

core::AppState TestTexture::onCleanup() {
	_textureShader.shutdown();
	_texture->shutdown();
	_texturedFullscreenQuad.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestTexture>()->startMainLoop(argc, argv);
}
