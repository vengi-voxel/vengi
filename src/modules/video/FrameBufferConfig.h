/**
 * @file
 */

#pragma once

#include "Types.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include "TextureConfig.h"
#include "core/collection/Map.h"

namespace video {

/**
 * @brief Configuration options or the @c FrameBuffer
 *
 * @sa FrameBuffer
 * @sa TextureConfig
 * @ingroup Video
 */
class FrameBufferConfig {
public:
	using ColorTextureMap = core::Map<FrameBufferAttachment, TextureConfig, 4, EnumClassHash>;
private:
	glm::ivec2 _dimension;
	ColorTextureMap _colorTextures;
	TextureFormat _colorTextureFormat = TextureFormat::RGBA;
	TextureFormat _depthTextureFormat = TextureFormat::D24;
	TextureFormat _depthBufferFormat = TextureFormat::D24;
	bool _colorTexture = false;
	bool _depthTexture = false;
	bool _depthBuffer = false;
	bool _stencilBuffer = false;
public:
	FrameBufferConfig& dimension(const glm::ivec2& dimension);
	const glm::ivec2& dimension() const;

	/**
	 * @brief Configure a default color attachment if no other was given
	 * @note Use the configured @c TextureFormat to set up the color texture attachment
	 * @sa colorTextureFormat()
	 */
	FrameBufferConfig& colorTexture(bool colorTexture);
	/**
	 * @brief Configure the default color texture attachment
	 * @sa colorTexture()
	 */
	FrameBufferConfig& colorTextureFormat(TextureFormat format);
	/**
	 * @return Returns the state of the default color texture
	 * @note If you configure custom color textures, this setting is ignored.
	 * @sa colorTexture()
	 */
	bool useColorTexture() const;
	/**
	 * @sa colorTextureFormat()
	 */
	TextureFormat colorTextureFormat() const;

	/**
	 * @brief Manually configure texture attachments
	 * @note If you add color texture attachements here, the @c colorTexture() state is ignored
	 */
	FrameBufferConfig& addTextureAttachment(const TextureConfig& cfg, video::FrameBufferAttachment attachment = video::FrameBufferAttachment::Color0);
	const ColorTextureMap& textureAttachments() const;

	/**
	 * @brief Enable or disable the depth texture binding
	 */
	FrameBufferConfig& depthTexture(bool depthTexture);
	FrameBufferConfig& depthTextureFormat(TextureFormat format);
	bool useDepthTexture() const;
	TextureFormat depthTextureFormat() const;

	/**
	 * @brief Enable or disable the depth buffer binding
	 */
	FrameBufferConfig& depthBuffer(bool depthBuffer);
	FrameBufferConfig& depthBufferFormat(TextureFormat format);
	bool useDepthBuffer() const;
	TextureFormat depthBufferFormat() const;

	/**
	 * @brief Enable or disable the stencil buffer binding
	 */
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

inline const FrameBufferConfig::ColorTextureMap& FrameBufferConfig::textureAttachments() const {
	return _colorTextures;
}

extern FrameBufferConfig defaultDepthBufferConfig(const glm::ivec2& dimension, int maxDepthBuffers);

}
