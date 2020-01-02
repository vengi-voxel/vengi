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

bool FrameBuffer::prepareAttachments(const FrameBufferConfig& cfg) {
	const glm::ivec2& dim = cfg.dimension();
	for (const auto& a : cfg.textureAttachments()) {
		_colorAttachments[a.first] = video::createTexture(a.second, dim.x, dim.y);
		if (a.first == FrameBufferAttachment::Depth) {
			_clearFlag |= ClearFlag::Depth;
		} else if (a.first == FrameBufferAttachment::DepthStencil) {
			_clearFlag |= ClearFlag::Depth;
			_clearFlag |= ClearFlag::Stencil;
		} else if (a.first == FrameBufferAttachment::Stencil) {
			_clearFlag |= ClearFlag::Stencil;
		} else {
			_clearFlag |= ClearFlag::Color;
		}
	}
	if (cfg.useColorTexture() && !_colorAttachments.count(FrameBufferAttachment::Color0) && !_bufferAttachments.count(FrameBufferAttachment::Color0)) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.colorTextureFormat());
		_colorAttachments[FrameBufferAttachment::Color0] = video::createTexture(textureCfg, dim.x, dim.y);
		_clearFlag |= ClearFlag::Color;
	}

	const bool depthStencil = _colorAttachments.count(FrameBufferAttachment::Depth)
					&& _bufferAttachments.count(FrameBufferAttachment::Depth)
					&& _colorAttachments.count(FrameBufferAttachment::DepthStencil)
					&& _bufferAttachments.count(FrameBufferAttachment::DepthStencil);
	if (cfg.useDepthTexture() && !depthStencil) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.depthTextureFormat());
		_colorAttachments[FrameBufferAttachment::Depth] = video::createTexture(textureCfg, dim.x, dim.y);
		_clearFlag |= ClearFlag::Depth;
	} else if (cfg.useDepthBuffer() && !depthStencil) {
		if (cfg.useStencilBuffer()) {
			_bufferAttachments[FrameBufferAttachment::DepthStencil] = video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y);
			_clearFlag |= ClearFlag::Depth;
			_clearFlag |= ClearFlag::Stencil;
		} else {
			_bufferAttachments[FrameBufferAttachment::Depth] = video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y);
			_clearFlag |= ClearFlag::Depth;
		}
	} else if (cfg.useStencilBuffer()) {
		_bufferAttachments[FrameBufferAttachment::Stencil] = video::createRenderBuffer(TextureFormat::S8, dim.x, dim.y);
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
		e.second->shutdown();
	}
	_colorAttachments.clear();
	for (auto& e : _bufferAttachments) {
		e.second->shutdown();
	}
	_bufferAttachments.clear();
}

TexturePtr FrameBuffer::texture(FrameBufferAttachment attachment) const {
	auto i = _colorAttachments.find(attachment);
	if (i == _colorAttachments.end()) {
		core_assert_msg(false, "Could not find framebuffer texture for %i", (int)attachment);
		return TexturePtr();
	}
	return i->second;
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
	auto i = _colorAttachments.find(attachment);
	if (i == _colorAttachments.end()) {
		Log::warn("Could not find texture attachment for attachment %i", (int)attachment);
		return false;
	}
	if (layerIndex < 0 || layerIndex >= i->second->layers()) {
		Log::warn("Given layer index (%i) is out of bounds: %i", layerIndex, (int)i->second->layers());
		return false;
	}
	return video::bindFrameBufferAttachment(i->second->handle(), attachment, layerIndex, clear);
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
