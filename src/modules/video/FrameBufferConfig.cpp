/**
 * @file
 */

#include "FrameBufferConfig.h"
#include "TextureConfig.h"
#include "core/Assert.h"

namespace video {

FrameBufferConfig& FrameBufferConfig::addTextureAttachment(const TextureConfig& cfg, video::FrameBufferAttachment attachment) {
	core_assert_msg(_colorTextures.count(attachment) == 0, "There is already a binding for the given attachment type");
	_colorTextures[attachment] = cfg;
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

FrameBufferConfig defaultDepthBufferConfig(const glm::ivec2& dimension, int maxDepthBuffers) {
	TextureConfig cfg;
	cfg.format(TextureFormat::D24S8).type(TextureType::Texture2DArray).wrap(TextureWrap::ClampToEdge);
	cfg.compareFunc(CompareFunc::Less).compareMode(TextureCompareMode::RefToTexture).layers(maxDepthBuffers);
	FrameBufferConfig fboCfg;
	fboCfg.dimension(dimension).colorTexture(false);
	fboCfg.addTextureAttachment(cfg, FrameBufferAttachment::Depth);
	return fboCfg;
}

}
