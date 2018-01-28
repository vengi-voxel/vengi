/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include "Texture.h"
#include <glm/vec2.hpp>

namespace video {

/**
 * @ingroup Video
 */
class DepthBuffer {
public:
	DepthBuffer();
	~DepthBuffer();

	bool init(const glm::ivec2& dimension, DepthBufferMode mode = DepthBufferMode::RGBA, int textureCount = 1);
	void shutdown();

	/**
	 * @brief Binds the depth framebuffer and updates the viewport to the framebuffer dimensions.
	 * @sa unbind()
	 */
	bool bind();
	bool bindTexture(int textureIndex);
	/**
	 * @brief Unbinds the depth framebuffer and restores the viewport that was active when bind() was called.
	 * @sa bind()
	 */
	void unbind();

	glm::ivec2 dimension() const;
	Id texture() const;
	TextureType textureType() const;
	bool depthAttachment() const;
	bool depthCompare() const;
private:
	int _oldViewport[4] = {0, 0, 0, 0};
	Id _oldFramebuffer = video::InvalidId;
	Id _fbo = video::InvalidId;
	Id _rbo = video::InvalidId;
	Texture _depthTexture;
	DepthBufferMode _mode = DepthBufferMode::RGBA;
};

inline glm::ivec2 DepthBuffer::dimension() const {
	return glm::ivec2(_depthTexture.width(), _depthTexture.height());
}

inline Id DepthBuffer::texture() const {
	return _depthTexture.handle();
}

inline TextureType DepthBuffer::textureType() const {
	return _depthTexture.type();
}

inline bool DepthBuffer::depthAttachment() const {
	return _mode == DepthBufferMode::DEPTH || depthCompare();
}

inline bool DepthBuffer::depthCompare() const {
	return _mode == DepthBufferMode::DEPTH_CMP;
}

inline bool bindTexture(TextureUnit unit, const DepthBuffer& depthBuffer) {
	video::bindTexture(unit, depthBuffer.textureType(), depthBuffer.texture());
	return true;
}

}
