#include "TestDepthBuffer.h"
#include "video/ScopedViewPort.h"

TestDepthBuffer::TestDepthBuffer(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
}

void TestDepthBuffer::doRender() {
	Super::doRender();
	const int width = _camera.width();
	const int height = _camera.height();
	const GLsizei quadWidth = (GLsizei) (width / 3.0f);
	const GLsizei quadHeight = (GLsizei) (height / 3.0f);
	video::ScopedShader scopedShader(_shadowMapRenderShader);
	video::ScopedViewPort scopedViewport(width - quadWidth, 0, quadWidth, quadHeight);
	core_assert_always(_texturedFullscreenQuad.bind());
	video::bindTexture(video::TextureUnit::Zero, _depthBuffer);
	_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
	video::drawArrays(video::Primitive::Triangles, _texturedFullscreenQuad.elements(0));
	_texturedFullscreenQuad.unbind();
}

core::AppState TestDepthBuffer::onInit() {
	core::AppState state = Super::onInit();
	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmaprender shader");
		return core::AppState::Cleanup;
	}
	const glm::ivec2& fullscreenQuadIndices = _texturedFullscreenQuad.createFullscreenTexturedQuad();

	video::VertexBuffer::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapRenderShader.getLocationPos();
	attributePos.size = _shadowMapRenderShader.getComponentsPos();
	_texturedFullscreenQuad.addAttribute(attributePos);

	video::VertexBuffer::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapRenderShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapRenderShader.getComponentsTexcoord();
	_texturedFullscreenQuad.addAttribute(attributeTexcoord);

	return state;
}

core::AppState TestDepthBuffer::onCleanup() {
	_texturedFullscreenQuad.shutdown();
	_shadowMapRenderShader.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestDepthBuffer app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
