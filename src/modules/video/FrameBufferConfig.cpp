/**
 * @file
 */

#include "FrameBufferConfig.h"

namespace video {

FrameBufferConfig& FrameBufferConfig::addColorTexture(const TextureConfig& cfg) {
	_colorTextures[FrameBufferAttachment::Color0] = cfg;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::colorTextureFormat(TextureFormat format) {
	_colorTextureFormat = format;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::depthTextureFormat(TextureFormat format) {
	_depthTextureFormat = format;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::depthBufferFormat(TextureFormat format) {
	_depthBufferFormat = format;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::dimension(const glm::ivec2& dimension) {
	_dimension = dimension;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::colorTexture(bool colorTexture) {
	_colorTexture = colorTexture;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::depthTexture(bool depthTexture) {
	_depthTexture = depthTexture;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::depthBuffer(bool depthBuffer) {
	_depthBuffer = depthBuffer;
	return *this;
}

FrameBufferConfig& FrameBufferConfig::stencilBuffer(bool stencilBuffer) {
	_stencilBuffer = stencilBuffer;
	return *this;
}

}
