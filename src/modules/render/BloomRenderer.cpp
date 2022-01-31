/**
 * @file
 */

#include "BloomRenderer.h"
#include "core/Log.h"
#include "video/Types.h"

namespace render {

bool BloomRenderer::init(bool yFlipped) {
	if (!_shader.setup()) {
		Log::error("Failed to init the bloom shader");
		return false;
	}

	const glm::ivec2 &fullscreenQuadIndices = _vbo.createFullscreenTexturedQuad(yFlipped);
	core_assert_always(_vbo.addAttribute(_shader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec2::x)));
	core_assert_always(_vbo.addAttribute(_shader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x)));
	return true;
}

void BloomRenderer::shutdown() {
	_shader.shutdown();
	_vbo.shutdown();
}

void BloomRenderer::render(video::Id color0, video::Id color1) {
	video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, color0);
	video::bindTexture(video::TextureUnit::One, video::TextureType::Texture2D, color1);

	video::ScopedShader scoped(_shader);
	core_assert_always(_shader.setColor0(video::TextureUnit::Zero));
	core_assert_always(_shader.setColor1(video::TextureUnit::One));

	video::ScopedBuffer scopedBuf(_vbo);
	const int elements = (int)_vbo.elements(0, _shader.getComponentsPos());
	core_assert_msg(elements == 6, "Unexpected amount of elements: %i", elements);

	video::drawArrays(video::Primitive::Triangles, elements);

	video::bindTexture(video::TextureUnit::One, video::TextureType::Texture2D, video::InvalidId);
	// ensure that texunit0 is active again
	video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, video::InvalidId);
}

} // namespace render
