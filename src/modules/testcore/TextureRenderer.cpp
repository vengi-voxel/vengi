/**
 * @file
 */

#include "TextureRenderer.h"
#include "core/Log.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"

namespace render {

bool TextureRenderer::init(bool yFlipped) {
	if (!_shader.setup()) {
		Log::error("Failed to init the texture shader");
		return false;
	}

	const math::Vec2i& fullscreenQuadIndices = _vbo.createFullscreenTexturedQuad(yFlipped);
	_vbo.addAttribute(_shader.getPosAttribute(fullscreenQuadIndices.x, &math::Vec2f::x));
	_vbo.addAttribute(_shader.getTexcoordAttribute(fullscreenQuadIndices.y, &math::Vec2f::x));
	return true;
}

void TextureRenderer::render(video::TextureUnit texUnit) {
	video::ScopedShader scoped(_shader);
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
