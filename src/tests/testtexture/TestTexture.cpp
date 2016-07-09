#include "TestTexture.h"
#include "core/Color.h"
#include "video/Camera.h"

TestTexture::TestTexture(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
}

core::AppState TestTexture::onInit() {
	_camera.setMode(video::CameraMode::Orthogonal);
	const core::AppState state = Super::onInit();
	_camera.setPosition(glm::vec3(_width / 2.0f, -_height / 2.0f, -50.0f));

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

	const glm::vec4& color = ::core::Color::White;
	glClearColor(color.r, color.g, color.b, color.a);

	return state;
}

void TestTexture::doRender() {
	video::ScopedShader scoped(_textureShader);
	_textureShader.setView(_camera.viewMatrix());
	_textureShader.setProjection(_camera.projectionMatrix());
	_textureShader.setModel(glm::scale(glm::mat4(), glm::vec3(_width / 2, _height / 2, 1.0)));
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
