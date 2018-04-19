/**
 * @file
 */

#pragma once

#include "Types.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include "TextureConfig.h"
#include <map>

namespace video {

/**
 * @brief Configuration options or the @c FrameBuffer
 */
class FrameBufferConfig {
private:
	bool _depth = false;
	glm::ivec2 _dimension;
	std::map<FrameBufferAttachment, TextureConfig> _colorTextures;
	TextureFormat _colorTextureFormat = TextureFormat::RGBA;
	TextureFormat _depthTextureFormat = TextureFormat::D24S8;
	TextureFormat _depthBufferFormat = TextureFormat::D24S8;
	bool _colorTexture = false;
	bool _depthTexture = false;
	bool _depthBuffer = false;
	bool _stencilBuffer = false;
public:
	FrameBufferConfig& dimension(const glm::ivec2& dimension);
	const glm::ivec2& dimension() const;

	FrameBufferConfig& colorTexture(bool colorTexture);
	FrameBufferConfig& colorTextureFormat(TextureFormat format);
	bool useColorTexture() const;
	TextureFormat colorTextureFormat() const;

	FrameBufferConfig& addColorTexture(const TextureConfig& cfg);
	const std::map<FrameBufferAttachment, TextureConfig>& colorTextures() const;

	FrameBufferConfig& depthTexture(bool depthTexture);
	FrameBufferConfig& depthTextureFormat(TextureFormat format);
	bool useDepthTexture() const;
	TextureFormat depthTextureFormat() const;

	FrameBufferConfig& depthBufferFormat(TextureFormat format);
	FrameBufferConfig& depthBuffer(bool depthBuffer);
	bool useDepthBuffer() const;
	TextureFormat depthBufferFormat() const;

	FrameBufferConfig& stencilBuffer(bool depthBuffer);
	bool useStencilBuffer() const;
};

inline const glm::ivec2& FrameBufferConfig::dimension() const {
	return _dimension;
}

inline bool FrameBufferConfig::useColorTexture() const {
	return _colorTexture;
}

inline bool FrameBufferConfig::useDepthTexture() const {
	return _depthTexture;
}

inline bool FrameBufferConfig::useDepthBuffer() const {
	return _depthBuffer;
}

inline bool FrameBufferConfig::useStencilBuffer() const {
	return _stencilBuffer;
}

inline TextureFormat FrameBufferConfig::colorTextureFormat() const {
	return _colorTextureFormat;
}

inline TextureFormat FrameBufferConfig::depthTextureFormat() const {
	return _depthTextureFormat;
}

inline TextureFormat FrameBufferConfig::depthBufferFormat() const {
	return _depthBufferFormat;
}

inline const std::map<FrameBufferAttachment, TextureConfig>& FrameBufferConfig::colorTextures() const {
	return _colorTextures;
}

}
