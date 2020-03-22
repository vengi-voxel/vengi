/**
 * @file
 */

#include "FrameBuffer.h"
#include "Texture.h"
#include "RenderBuffer.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "Renderer.h"

namespace video {

FrameBuffer::~FrameBuffer() {
	core_assert_msg(_fbo == video::InvalidId, "Framebuffer was not properly shut down");
	shutdown();
}

void FrameBuffer::addColorAttachment(FrameBufferAttachment attachment, const TexturePtr& texture) {
	_colorAttachments[core::enumVal(attachment)] = texture;
}

bool FrameBuffer::hasColorAttachment(FrameBufferAttachment attachment) {
	return (bool)_colorAttachments[core::enumVal(attachment)];
}

void FrameBuffer::addBufferAttachment(FrameBufferAttachment attachment, const RenderBufferPtr& renderBuffer) {
	_bufferAttachments[core::enumVal(attachment)] = renderBuffer;
}

bool FrameBuffer::hasBufferAttachment(FrameBufferAttachment attachment) {
	return (bool)_bufferAttachments[core::enumVal(attachment)];
}

bool FrameBuffer::prepareAttachments(const FrameBufferConfig& cfg) {
	const glm::ivec2& dim = cfg.dimension();
	for (const auto& a : cfg.textureAttachments()) {
		const FrameBufferAttachment key = a->key;
		const TextureConfig& textureConfig = a->value;
		addColorAttachment(key, video::createTexture(textureConfig, dim.x, dim.y));
		if (key == FrameBufferAttachment::Depth) {
			_clearFlag |= ClearFlag::Depth;
		} else if (key == FrameBufferAttachment::DepthStencil) {
			_clearFlag |= ClearFlag::Depth;
			_clearFlag |= ClearFlag::Stencil;
		} else if (key == FrameBufferAttachment::Stencil) {
			_clearFlag |= ClearFlag::Stencil;
		} else {
			_clearFlag |= ClearFlag::Color;
		}
	}
	if (cfg.useColorTexture() && !hasColorAttachment(FrameBufferAttachment::Color0) && !hasBufferAttachment(FrameBufferAttachment::Color0)) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.colorTextureFormat());
		_colorAttachments[core::enumVal(FrameBufferAttachment::Color0)] = video::createTexture(textureCfg, dim.x, dim.y);
		_clearFlag |= ClearFlag::Color;
	}

	const bool depthStencil = hasColorAttachment(FrameBufferAttachment::Depth)
					&& hasBufferAttachment(FrameBufferAttachment::Depth)
					&& hasColorAttachment(FrameBufferAttachment::DepthStencil)
					&& hasBufferAttachment(FrameBufferAttachment::DepthStencil);
	if (cfg.useDepthTexture() && !depthStencil) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.depthTextureFormat());
		addColorAttachment(FrameBufferAttachment::Depth, video::createTexture(textureCfg, dim.x, dim.y));
		_clearFlag |= ClearFlag::Depth;
	} else if (cfg.useDepthBuffer() && !depthStencil) {
		if (cfg.useStencilBuffer()) {
			addBufferAttachment(FrameBufferAttachment::DepthStencil, video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y));
			_clearFlag |= ClearFlag::Depth;
			_clearFlag |= ClearFlag::Stencil;
		} else {
			addBufferAttachment(FrameBufferAttachment::Depth, video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y));
			_clearFlag |= ClearFlag::Depth;
		}
	} else if (cfg.useStencilBuffer()) {
		addBufferAttachment(FrameBufferAttachment::Stencil, video::createRenderBuffer(TextureFormat::S8, dim.x, dim.y));
		_clearFlag |= ClearFlag::Stencil;
	}

	return true;
}

bool FrameBuffer::init(const FrameBufferConfig& cfg) {
	_dimension = cfg.dimension();
	_fbo = genFramebuffer();
	Id prev = video::bindFramebuffer(_fbo);
	bool retVal = prepareAttachments(cfg);
	if (retVal) {
		retVal = video::setupFramebuffer(_colorAttachments, _bufferAttachments);
	}
	video::bindFramebuffer(prev);
	return retVal;
}

glm::vec4 FrameBuffer::uv() const {
	return video::framebufferUV();
}

void FrameBuffer::shutdown() {
	video::deleteFramebuffer(_fbo);
	for (auto& e : _colorAttachments) {
		if (!e) {
			continue;
		}
		e->shutdown();
	}
	for (auto& e : _bufferAttachments) {
		if (!e) {
			continue;
		}
		e->shutdown();
	}
}

TexturePtr FrameBuffer::texture(FrameBufferAttachment attachment) const {
	auto tex = _colorAttachments[core::enumVal(attachment)];
	if (!tex) {
		core_assert_msg(false, "Could not find framebuffer texture for %i", (int)attachment);
	}
	return tex;
}

void FrameBuffer::bind(bool clear) {
	video::getViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::viewport(0, 0, _dimension.x, _dimension.y);
	_oldFramebuffer = video::bindFramebuffer(_fbo);
	if (clear) {
		video::clear(_clearFlag);
	}
}

bool FrameBuffer::bindTextureAttachment(FrameBufferAttachment attachment, int layerIndex, bool clear) {
	auto tex = _colorAttachments[core::enumVal(attachment)];
	if (!tex) {
		Log::warn("Could not find texture attachment for attachment %i", (int)attachment);
		return false;
	}
	if (layerIndex < 0 || layerIndex >= tex->layers()) {
		Log::warn("Given layer index (%i) is out of bounds: %i", layerIndex, (int)tex->layers());
		return false;
	}
	return video::bindFrameBufferAttachment(tex->handle(), attachment, layerIndex, clear);
}

void FrameBuffer::unbind() {
	video::viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::bindFramebuffer(_oldFramebuffer);
	_oldFramebuffer = InvalidId;
}

bool bindTexture(TextureUnit unit, const FrameBuffer& frameBuffer, FrameBufferAttachment attachment) {
	const TexturePtr& tex = frameBuffer.texture(attachment);
	if (!tex) {
		return false;
	}
	video::bindTexture(unit, tex->type(), tex->handle());
	return true;
}

}
