/**
 * @file
 */

#include "BlurRenderer.h"
#include "core/Log.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/ScopedFrameBuffer.h"
#include "video/ScopedViewPort.h"
#include "video/Types.h"

namespace render {

bool BlurRenderer::init(bool yFlipped, int width, int height) {
	if (!_shader.setup()) {
		Log::error("Failed to init the blur shader");
		return false;
	}

	for (int i = 0; i < lengthof(_frameBuffers); ++i) {
		video::FrameBufferConfig cfg;
		cfg.dimension(glm::ivec2(width, height));
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
		if (!_frameBuffers[i].init(cfg)) {
			Log::error("Failed to init the blur framebuffer %i", i);
			return false;
		}
	}
	const glm::ivec2 &fullscreenQuadIndices = _vbo.createFullscreenTexturedQuad(yFlipped);
	core_assert_always(_vbo.addAttribute(_shader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec2::x)));
	core_assert_always(_vbo.addAttribute(_shader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x)));
	return true;
}

void BlurRenderer::render(video::Id srcTextureId, int amount) {
	if (amount <= 0) {
		const int indexFlip = _horizontal ? 0 : 1;
		video::ScopedFrameBuffer scoped(_frameBuffers[indexFlip]);
		video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
		return;
	}

	core_assert(srcTextureId != video::InvalidId);

	_horizontal = true;
	bool firstIteration = true;
	const video::TextureUnit texUnit = video::TextureUnit::Zero;

	video::ScopedShader scoped(_shader);
	core_assert_always(_shader.setImage(texUnit));

	video::ScopedBuffer scopedBuf(_vbo);
	const int elements = (int)_vbo.elements(0, _shader.getComponentsPos());
	core_assert_msg(elements == 6, "Unexpected amount of elements: %i", elements);

	// we have to keep the runs even, due to the framebuffer color texture attachment y flips
	// for opengl
	const int n = glm::max(2, amount / 2 * 2);
	for (int i = 0; i < n; i++) {
		const int index = _horizontal ? 1 : 0;
		const int indexFlip = _horizontal ? 0 : 1;
		_frameBuffers[index].bind(true);
		core_assert_always(_shader.setHorizontal(_horizontal));
		const video::TexturePtr& srcTexture = _frameBuffers[indexFlip].texture(video::FrameBufferAttachment::Color0);
		video::Id srcTextureHandle = srcTexture->handle();
		core_assert(srcTextureHandle != video::InvalidId);
		if (firstIteration) {
			// the first iteration uses the given textureId - normally from a framebuffer color texture attachment
			// were we previously rendered to
			srcTextureHandle = srcTextureId;
		}
		video::bindTexture(texUnit, video::TextureType::Texture2D, srcTextureHandle);
		video::drawArrays(video::Primitive::Triangles, elements);
		_horizontal = !_horizontal;
		firstIteration = false;
		_frameBuffers[index].unbind();
	}
	video::bindTexture(texUnit, video::TextureType::Texture2D, video::InvalidId);
}

video::TexturePtr BlurRenderer::texture() const {
	const int indexFlip = _horizontal ? 0 : 1;
	return _frameBuffers[indexFlip].texture(video::FrameBufferAttachment::Color0);
}

void BlurRenderer::shutdown() {
	for (int i = 0; i < lengthof(_frameBuffers); ++i) {
		_frameBuffers[i].shutdown();
	}
	_shader.shutdown();
	_vbo.shutdown();
}

} // namespace render
