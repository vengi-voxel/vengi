/**
 * @file
 */

#include "TextureRenderer.h"
#include "core/Log.h"

namespace render {

bool TextureRenderer::init(bool yFlipped) {
	if (!_shader.setup()) {
		Log::error("Failed to init the texture shader");
		return false;
	}

	const glm::ivec2& fullscreenQuadIndices = _vbo.createFullscreenTexturedQuad(yFlipped);
	_vbo.addAttribute(_shader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec2::x));
	_vbo.addAttribute(_shader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x));
	return true;
}

void TextureRenderer::render(video::TextureUnit texUnit) {
	video::ScopedShader scoped(_shader);
	_shader.setTexture(texUnit);
	video::ScopedBuffer scopedBuf(_vbo);
	const int elements = (int)_vbo.elements(0, _shader.getComponentsPos());
	core_assert(elements == 6);
	video::drawArrays(video::Primitive::Triangles, elements);
}

void TextureRenderer::shutdown() {
	_shader.shutdown();
	_vbo.shutdown();
}

}
