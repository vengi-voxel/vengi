/**
 * @file
 */

#include "SurfaceOverlayRenderer.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/ScopedBlendMode.h"
#include "video/ScopedState.h"
#include "video/Shader.h"

namespace render {

bool SurfaceOverlayRenderer::resize(const glm::ivec2 &size) {
	if (_surfaces.handle() != video::InvalidId && _sceneDepth.handle() != video::InvalidId &&
		_surfaces.dimension() == size && _sceneDepth.dimension() == size) {
		return true;
	}
	_surfaces.shutdown();
	_sceneDepth.shutdown();
	video::FrameBufferConfig cfg;
	cfg.dimension(size).depthTexture(true).depthTextureFormat(video::TextureFormat::D32F);
	if (!_sceneDepth.init(cfg)) {
		return false;
	}
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1);
	return _surfaces.init(cfg);
}

bool SurfaceOverlayRenderer::begin(const glm::ivec2 &size) {
	if (size.x <= 0 || size.y <= 0) {
		return false;
	}
	const video::Id target = video::currentFramebuffer();
	if (target == video::InvalidId) {
		return false;
	}
	if (_position == -1) {
		if (!_shader.setup()) {
			return false;
		}
		if (!_data.create(_fragData)) {
			shutdown();
			return false;
		}
		_position = _quad.createFullscreenQuad();
		if (_position == -1 || !_quad.addAttribute(_shader.getPosAttribute(_position, &glm::vec2::x))) {
			shutdown();
			return false;
		}
	}
	const bool ready = resize(size);
	video::bindFramebuffer(target);
	if (!ready) {
		return false;
	}
	const video::ScopedState scissor(video::State::Scissor, false);
	const video::ScopedState depthMask(video::State::DepthMask, true);
	video::blitFramebuffer(target, _sceneDepth.handle(), video::ClearFlag::Depth, size.x, size.y);
	const glm::vec4 previousColor = video::currentClearColor();
	video::clearColor(glm::vec4(0.0f));
	_surfaces.bind(true);
	video::clearColor(previousColor);
	return true;
}

void SurfaceOverlayRenderer::end(const glm::vec4 &tint) {
	_surfaces.unbind();
	const video::FrameBufferAttachment color0[] = {video::FrameBufferAttachment::Color0};
	video::drawBuffers(1, color0);
	const video::ScopedState depth(video::State::DepthTest, false);
	const video::ScopedState depthMask(video::State::DepthMask, false);
	const video::ScopedState scissor(video::State::Scissor, false);
	const video::ScopedState cull(video::State::CullFace, false);
	const video::ScopedBlendMode blend(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha,
									   video::BlendEquation::Add);
	const video::ScopedShader shader(_shader);
	_fragData.tint = tint;
	core_assert_always(_data.update(_fragData));
	core_assert_always(_shader.setFrag(_data.getFragUniformBuffer()));
	video::bindTexture(video::TextureUnit::Zero, _surfaces, video::FrameBufferAttachment::Color0);
	video::bindTexture(video::TextureUnit::One, _surfaces, video::FrameBufferAttachment::Depth);
	video::bindTexture(video::TextureUnit::Two, _sceneDepth, video::FrameBufferAttachment::Depth);
	if (_quad.bind()) {
		video::drawArrays(video::Primitive::Triangles, 6);
		_quad.unbind();
	}
}

void SurfaceOverlayRenderer::shutdown() {
	_quad.shutdown();
	_shader.shutdown();
	_data.shutdown();
	_position = -1;
	_surfaces.shutdown();
	_sceneDepth.shutdown();
}

} // namespace render
