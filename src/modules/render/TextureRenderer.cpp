/**
 * @file
 */

#include "TextureRenderer.h"

namespace render {

bool TextureRenderer::init(const glm::vec2& size) {
	if (!_textureShader.setup()) {
		Log::error("Failed to init the texture shader");
		return false;
	}

	const glm::ivec2& fullscreenQuadIndices = _texturedFullscreenQuad.createTexturedQuad(glm::vec2(0.0f), size);
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

	return true;
}

void TextureRenderer::render(const glm::mat4& projection) {
	video::ScopedShader scoped(_textureShader);
	_textureShader.setProjection(projection);
	_textureShader.setTexture(video::TextureUnit::Zero);
	video::ScopedBuffer scopedBuf(_texturedFullscreenQuad);
	const int elements = _texturedFullscreenQuad.elements(0, _textureShader.getComponentsPos());
	video::drawArrays(video::Primitive::Triangles, elements);
}

void TextureRenderer::shutdown() {

}

}
