/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <memory>

namespace video {

typedef std::shared_ptr<Texture> TexturePtr;

/**
 * @ingroup Video
 */
class DepthBuffer {
public:
	~DepthBuffer();

	bool init(const glm::ivec2& dimension, int textureCount);
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
private:
	int _oldViewport[4] = {0, 0, 0, 0};
	Id _oldFramebuffer = video::InvalidId;
	Id _fbo = video::InvalidId;
	Id _rbo = video::InvalidId;
	TexturePtr _depthTexture;
};

inline bool bindTexture(TextureUnit unit, const DepthBuffer& depthBuffer) {
	video::bindTexture(unit, depthBuffer.textureType(), depthBuffer.texture());
	return true;
}

}
