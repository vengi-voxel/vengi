/**
 * @file
 */
#include "TestGLSLComp.h"
#include "io/Filesystem.h"
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
	_camera.setMode(video::CameraMode::Orthogonal);
	_camera.setNearPlane(-1.0f);
	_camera.setFarPlane(1.0f);

	if (!_testShader.setup()) {
		Log::error("Failed to init the compute shader");
		return core::AppState::InitFailure;
	}

	if (!_textureShader.setup()) {
		Log::error("Failed to init the texture shader");
		return core::AppState::InitFailure;
	}

	_texture = std::make_shared<video::Texture>(video::TextureType::Texture2D, video::TextureFormat::RGBA32F, appname(), 512, 512);
	_texture->upload(nullptr);
	video::bindImage(_texture->handle(), video::AccessMode::Write, _testShader.getImageFormatImgOutput());

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

core::AppState TestGLSLComp::onCleanup() {
	core::AppState state = Super::onCleanup();
	_testShader.shutdown();
	_textureShader.shutdown();
	if (_texture) {
		_texture->shutdown();
	}
	_texturedFullscreenQuad.shutdown();
	return state;
}

void TestGLSLComp::doRender() {
	_testShader.activate();
	_testShader.run(glm::uvec3(_texture->width(), _texture->height(), 1), true);

	video::ScopedViewPort viewPort(0, 0, dimension().x, dimension().y);
	video::ScopedShader scoped(_textureShader);
	_textureShader.setProjection(_camera.projectionMatrix());
	_textureShader.setTexture(video::TextureUnit::Zero);
	_texture->bind(video::TextureUnit::Zero);
	_texturedFullscreenQuad.bind();
	const int elements = _texturedFullscreenQuad.elements(0, _textureShader.getComponentsPos());
	video::drawArrays(video::Primitive::Triangles, elements);
	_texturedFullscreenQuad.unbind();
	_texture->unbind();
}

TEST_APP(TestGLSLComp)
