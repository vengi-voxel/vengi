/**
 * @file
 */

#include "DepthBuffer.h"
#include "Renderer.h"
#include "ScopedFrameBuffer.h"
#include "Texture.h"

#include <cstddef>
#include "core/Common.h"

namespace video {

DepthBuffer::DepthBuffer() :
		_depthTexture(TextureType::Texture2DArray, TextureFormat::D24S8, "**depthmap**", 1, 1, 1, TextureWrap::ClampToEdge) {
}

DepthBuffer::~DepthBuffer() {
	core_assert_msg(_fbo == InvalidId, "Depthbuffer was not properly shut down");
	shutdown();
}

void DepthBuffer::shutdown() {
	video::deleteFramebuffer(_fbo);
	_depthTexture.shutdown();
	video::deleteRenderbuffer(_rbo);
	core_assert(_oldFramebuffer == video::InvalidId);
}

bool DepthBuffer::init(const glm::ivec2& dimension, DepthBufferMode mode, int textureCount) {
	if (textureCount > 4 || textureCount < 1) {
		Log::error("Invalid texture count for depthbuffer: %i", textureCount);
		return false;
	}
	_mode = mode;
	_fbo = video::genFramebuffer();
	TextureFormat format;
	const bool depthCompare = mode == DepthBufferMode::DEPTH_CMP;
	const bool depthAttachment = mode == DepthBufferMode::DEPTH || depthCompare;
	if (depthAttachment) {
		format = TextureFormat::D24S8;
	} else {
		format = TextureFormat::RGBA;
	}
	_depthTexture.upload(format, dimension.x, dimension.y, nullptr, textureCount);
	if (depthCompare) {
		const TextureType type = textureType();
		video::setupDepthCompareTexture(TextureUnit::Upload, type, _depthTexture);
	}
	return video::setupDepthbuffer(_fbo, _mode);
}

bool DepthBuffer::bind() {
	video::getViewport(_oldViewport[0], _oldViewport[1], _oldViewport[2], _oldViewport[3]);
	_oldFramebuffer = video::bindFramebuffer(video::FrameBufferMode::Default, _fbo);
	video::viewport(0, 0, _depthTexture.width(), _depthTexture.height());
	return true;
}

bool DepthBuffer::bindTexture(int textureIndex) {
	core_assert(textureIndex >= 0 && textureIndex < 4);
	if (textureIndex < 0 || textureIndex >= 4) {
		return false;
	}
	return video::bindDepthTexture(textureIndex, _mode, _depthTexture);
}

void DepthBuffer::unbind() {
	video::viewport(_oldViewport[0], _oldViewport[1], _oldViewport[2], _oldViewport[3]);
	video::bindFramebuffer(video::FrameBufferMode::Default, _oldFramebuffer);
	_oldFramebuffer = video::InvalidId;
}

}
