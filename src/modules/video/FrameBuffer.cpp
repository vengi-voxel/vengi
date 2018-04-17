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
	}
	if (cfg.useColorTexture() && !_colorAttachments.count(FrameBufferAttachment::Color0) && !_bufferAttachments.count(FrameBufferAttachment::Color0)) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.colorTextureFormat());
		_colorAttachments[FrameBufferAttachment::Color0] = video::createTexture(textureCfg, dim.x, dim.y);
	}

	const bool depthStencil = _colorAttachments.count(FrameBufferAttachment::Depth)
					&& _bufferAttachments.count(FrameBufferAttachment::Depth)
					&& _colorAttachments.count(FrameBufferAttachment::DepthStencil)
					&& _bufferAttachments.count(FrameBufferAttachment::DepthStencil);
	if (cfg.useDepthTexture() && !depthStencil) {
		TextureConfig textureCfg;
		textureCfg.format(cfg.depthTextureFormat());
		_colorAttachments[FrameBufferAttachment::Depth] = video::createTexture(textureCfg, dim.x, dim.y);
	} else if (cfg.useDepthBuffer() && !depthStencil) {
		if (cfg.useStencilBuffer()) {
			_bufferAttachments[FrameBufferAttachment::DepthStencil] = video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y);
		} else {
			_bufferAttachments[FrameBufferAttachment::Depth] = video::createRenderBuffer(cfg.depthBufferFormat(), dim.x, dim.y);
		}
	} else if (cfg.useStencilBuffer()) {
		_bufferAttachments[FrameBufferAttachment::Stencil] = video::createRenderBuffer(TextureFormat::S8, dim.x, dim.y);
	}

	return true;
}

bool FrameBuffer::init(const glm::ivec2& dimension) {
	_dimension = dimension;
	_fbo = genFramebuffer();

	video::TextureConfig textureCfg;
	textureCfg.wrap(TextureWrap::ClampToEdge);
	Id prev = video::bindFramebuffer(FrameBufferMode::Default, _fbo);
	FrameBufferConfig cfg;
	cfg.dimension(_dimension).depthBuffer(true).depthBufferFormat(TextureFormat::D24).addColorTexture(textureCfg);
	bool retVal = prepareAttachments(cfg);
	if (retVal) {
		retVal = video::setupFramebuffer(_colorAttachments, _bufferAttachments);
	}
	video::bindFramebuffer(FrameBufferMode::Default, prev);
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

void FrameBuffer::bind() {
	video::getViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::viewport(0, 0, _dimension.x, _dimension.y);
	_oldFramebuffer = video::bindFramebuffer(FrameBufferMode::Default, _fbo);
	video::clear(ClearFlag::Color | ClearFlag::Depth);
}

void FrameBuffer::unbind() {
	video::viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::bindFramebuffer(FrameBufferMode::Default, _oldFramebuffer);
	_oldFramebuffer = InvalidId;
}

}
