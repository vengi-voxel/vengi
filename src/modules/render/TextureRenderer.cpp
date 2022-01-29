/**
 * @file
 */

#include "TextureRenderer.h"
#include "core/Log.h"

namespace render {

bool TextureRenderer::init(const glm::vec2& size, bool yFlipped) {
	if (!_shader.setup()) {
		Log::error("Failed to init the texture shader");
		return false;
	}

	const glm::ivec2& fullscreenQuadIndices = _vbo.createTexturedQuad(glm::vec2(0.0f), size, yFlipped);
	_vbo.addAttribute(_shader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec2::x));
	_vbo.addAttribute(_shader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x));
	_vbo.addAttribute(_shader.getColorAttribute(_vbo.createWhiteColorForQuad(), &glm::vec2::x));
	return true;
}

void TextureRenderer::render(const glm::mat4& projection, const glm::mat4& model, video::TextureUnit texUnit) {
	video::ScopedShader scoped(_shader);
	_shader.setViewprojection(projection);
	_shader.setModel(model);
	_shader.setTexture(texUnit);
	video::ScopedBuffer scopedBuf(_vbo);
	const int elements = (int)_vbo.elements(0, _shader.getComponentsPos());
	video::drawArrays(video::Primitive::Triangles, elements);
}

void TextureRenderer::shutdown() {
	_shader.shutdown();
	_vbo.shutdown();
}

}
