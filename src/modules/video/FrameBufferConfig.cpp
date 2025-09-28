/**
 * @file
 */

#include "FrameBufferConfig.h"
#include "TextureConfig.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "video/Renderer.h"
#include <glm/common.hpp>

namespace video {

FrameBufferConfig& FrameBufferConfig::addTextureAttachment(const TextureConfig& cfg, video::FrameBufferAttachment attachment) {
	core_assert_msg(_colorTextures.find(attachment) == _colorTextures.end(), "There is already a binding for the given attachment type");
	_colorTextures.put(attachment, cfg);
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
	if (dimension.x <= 0 || dimension.y <= 0) {
		Log::error("Invalid dimension for framebuffer: w: %i, h: %i", dimension.x, dimension.y);
	}
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

FrameBufferConfig& FrameBufferConfig::samples(int samples) {
	_samples = glm::clamp(samples, 0, video::limiti(Limit::MaxSamples));
	return *this;
}

FrameBufferConfig defaultDepthBufferConfig(const glm::ivec2& dimension, int maxDepthBuffers) {
	TextureConfig cfg;
	cfg.format(TextureFormat::D32F).type(TextureType::Texture2DArray);
	cfg.compareFunc(CompareFunc::Less).compareMode(TextureCompareMode::RefToTexture).layers(maxDepthBuffers);
	// coordinates outside the depth buffer will result in no shadow
	cfg.borderColor(glm::vec4(1.0f));
	cfg.wrap(TextureWrap::ClampToBorder);

	FrameBufferConfig fboCfg;
	fboCfg.dimension(dimension).colorTexture(false);
	fboCfg.addTextureAttachment(cfg, FrameBufferAttachment::Depth);
	return fboCfg;
}

}
