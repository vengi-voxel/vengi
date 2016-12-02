#include "TestTexture.h"
#include "core/Color.h"
#include "video/Camera.h"

TestTexture::TestTexture(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
}

core::AppState TestTexture::onInit() {
	const core::AppState state = Super::onInit();
	_camera.setMode(video::CameraMode::Orthogonal);
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

	const glm::ivec2& fullscreenQuadIndices = _texturedFullscreenQuad.createTexturedQuad(glm::vec2(0), dimension());
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
	glViewport(0, 0, dimension().x, dimension().y);
	video::ScopedShader scoped(_textureShader);
	const glm::mat4& ortho = glm::ortho(0.0f, (float) dimension().x, (float) dimension().y, 0.0f, -1.0f, 1.0f);
	_textureShader.setProjection(ortho);
	_texture->bind();
	_texturedFullscreenQuad.bind();
	const int elements = _texturedFullscreenQuad.elements(0, 2);
	glDrawArrays(GL_TRIANGLES, 0, elements);
	_texturedFullscreenQuad.unbind();
	_texture->unbind();
	GL_checkError();
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
