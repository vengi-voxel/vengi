/**
 * @file
 */

#include "FrameBuffer.h"
#include "Texture.h"
#include "RenderBuffer.h"

namespace video {

FrameBuffer::~FrameBuffer() {
	core_assert_msg(_fbo == video::InvalidId, "Framebuffer was not properly shut down");
	shutdown();
}

bool FrameBuffer::prepareAttachments(const FrameBufferConfig& cfg) {
	const glm::ivec2& dim = cfg.dimension();
	for (const auto& a : cfg.colorTextures()) {
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

Id FrameBuffer::texture() const {
	auto i = _colorAttachments.find(FrameBufferAttachment::Color0);
	if (i == _colorAttachments.end()) {
		return InvalidId;
	}
	return i->second->handle();
}

void FrameBuffer::bind(bool clear) {
	video::getViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::viewport(0, 0, _dimension.x, _dimension.y);
	_oldFramebuffer = video::bindFramebuffer(_fbo);
	if (clear) {
		video::clear(_clearFlag);
	}
}

void FrameBuffer::unbind() {
	video::viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::bindFramebuffer(_oldFramebuffer);
	_oldFramebuffer = InvalidId;
}

}
