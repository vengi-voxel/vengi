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

	if (!_textureShader.setup()) {
		Log::error("Failed to init the texture shader");
		return core::AppState::InitFailure;
	}

	_texture = video::createTextureFromImage("texture.png");
	if (!_texture) {
		Log::error("Failed to load texture");
		return core::AppState::InitFailure;
	}

	const glm::ivec2& fullscreenQuadIndices = _texturedFullscreenQuad.createTexturedQuad(glm::vec2(0), dimension());
	video::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _textureShader.getLocationPos();
	attributePos.size = _textureShader.getComponentsPos();
	_texturedFullscreenQuad.addAttribute(attributePos);

	video::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _textureShader.getLocationTexcoord();
	attributeTexcoord.size = _textureShader.getComponentsTexcoord();
	_texturedFullscreenQuad.addAttribute(attributeTexcoord);

	video::Attribute attributeColor;
	attributeColor.bufferIndex = _texturedFullscreenQuad.createWhiteColorForQuad();
	attributeColor.index = _textureShader.getLocationColor();
	attributeColor.size = _textureShader.getComponentsColor();
	_texturedFullscreenQuad.addAttribute(attributeColor);

	video::clearColor(::core::Color::White);

	return state;
}

void TestTexture::doRender() {
	video::ScopedViewPort viewPort(0, 0, dimension().x, dimension().y);
	video::ScopedShader scoped(_textureShader);
	_textureShader.setProjection(_camera.projectionMatrix());
	_texture->bind();
	_texturedFullscreenQuad.bind();
	const int elements = _texturedFullscreenQuad.elements(0, _textureShader.getComponentsPos());
	video::drawArrays(video::Primitive::Triangles, elements);
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

TEST_APP(TestTexture)
