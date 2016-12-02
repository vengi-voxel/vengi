#include "TestTexture.h"
#include "core/Color.h"
#include "video/Camera.h"

TestTexture::TestTexture(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
}

core::AppState TestTexture::onInit() {
	_camera.setMode(video::CameraMode::Orthogonal);
	const core::AppState state = Super::onInit();
	_camera.setPosition(glm::vec3(_dimension.x / 2.0f, -_dimension.y / 2.0f, -50.0f));

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
	video::VertexBuffer::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _textureShader.getLocationPos();
	attributePos.size = _textureShader.getComponentsPos();
	_texturedFullscreenQuad.addAttribute(attributePos);

	video::VertexBuffer::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _textureShader.getLocationTexcoord();
	attributeTexcoord.size = _textureShader.getComponentsTexcoord();
	_texturedFullscreenQuad.addAttribute(attributeTexcoord);

	const glm::vec4& color = ::core::Color::White;
	glClearColor(color.r, color.g, color.b, color.a);

	return state;
}

void TestTexture::doRender() {
	video::ScopedShader scoped(_textureShader);
	_textureShader.setView(_camera.viewMatrix());
	_textureShader.setProjection(_camera.projectionMatrix());
	_textureShader.setModel(glm::scale(glm::mat4(), glm::vec3(_dimension.x / 2, _dimension.y / 2, 1.0)));
	_textureShader.setTexture(0);
	_texture->bind();
	core_assert_always(_texturedFullscreenQuad.bind());
	glDrawArrays(GL_TRIANGLES, 0, _texturedFullscreenQuad.elements(0));
	_texturedFullscreenQuad.unbind();
	_texture->unbind();
}

core::AppState TestTexture::onCleanup() {
	_textureShader.shutdown();
	if (_texture) {
		_texture->shutdown();
	}
	_texturedFullscreenQuad.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	TestTexture app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);

}
