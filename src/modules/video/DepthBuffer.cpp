/**
 * @file
 */

#include "DepthBuffer.h"
#include "Renderer.h"
#include "ScopedFrameBuffer.h"
#include "Texture.h"

#include <stddef.h>
#include "core/Common.h"

namespace video {

DepthBuffer::~DepthBuffer() {
	core_assert_msg(_fbo == InvalidId, "Depthbuffer was not properly shut down");
	shutdown();
}

void DepthBuffer::shutdown() {
	video::deleteFramebuffer(_fbo);
	if (_depthTexture) {
		_depthTexture->shutdown();
	}
	video::deleteRenderbuffer(_rbo);
	core_assert(_oldFramebuffer == video::InvalidId);
}

bool DepthBuffer::init(const glm::ivec2& dimension, int textureCount) {
	if (textureCount > 4 || textureCount < 1) {
		Log::error("Invalid texture count for depthbuffer: %i", textureCount);
		return false;
	}
	_fbo = video::genFramebuffer();
	TextureConfig cfg;
	cfg.format(TextureFormat::D24S8).type(TextureType::Texture2DArray).wrap(TextureWrap::ClampToEdge);
	_depthTexture = createTexture(cfg, 1, 1, "**depthmap**");
	_depthTexture->upload(cfg.format(), dimension.x, dimension.y, nullptr, textureCount);
	video::setupDepthCompareTexture(_depthTexture->type(), CompareFunc::Less, TextureCompareMode::RefToTexture);
	const Id prev = bindFramebuffer(FrameBufferMode::Default, _fbo);
	const bool retVal = video::setupDepthbuffer();
	bindFramebuffer(FrameBufferMode::Default, prev);
	return retVal;
}

bool DepthBuffer::bind() {
	video::getViewport(_oldViewport[0], _oldViewport[1], _oldViewport[2], _oldViewport[3]);
	_oldFramebuffer = video::bindFramebuffer(video::FrameBufferMode::Default, _fbo);
	video::viewport(0, 0, _depthTexture->width(), _depthTexture->height());
	return true;
}

bool DepthBuffer::bindTexture(int textureIndex) {
	core_assert(textureIndex >= 0 && textureIndex < 4);
	if (textureIndex < 0 || textureIndex >= 4) {
		return false;
	}
	return video::bindDepthTexture(textureIndex, *_depthTexture);
}

void DepthBuffer::unbind() {
	video::viewport(_oldViewport[0], _oldViewport[1], _oldViewport[2], _oldViewport[3]);
	video::bindFramebuffer(video::FrameBufferMode::Default, _oldFramebuffer);
	_oldFramebuffer = video::InvalidId;
}

glm::ivec2 DepthBuffer::dimension() const {
	return glm::ivec2(_depthTexture->width(), _depthTexture->height());
}

Id DepthBuffer::texture() const {
	return _depthTexture->handle();
}

TextureType DepthBuffer::textureType() const {
	return _depthTexture->type();
}

}
