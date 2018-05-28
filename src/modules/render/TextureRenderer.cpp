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
	_texturedFullscreenQuad.addAttribute(_textureShader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec2::x));
	_texturedFullscreenQuad.addAttribute(_textureShader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x));
	_texturedFullscreenQuad.addAttribute(_textureShader.getColorAttribute(_texturedFullscreenQuad.createWhiteColorForQuad(), &glm::vec2::x));
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
	_textureShader.shutdown();
	_texturedFullscreenQuad.shutdown();
}

}
