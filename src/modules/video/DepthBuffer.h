/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include "Texture.h"
#include <glm/vec2.hpp>

namespace video {

enum class DepthBufferMode {
	// stores -1..1 window-space depth values
	RGBA,
	// stores 0..1 window-space depth values
	DEPTH,
	DEPTH_CMP
};

class DepthBuffer {
public:
	DepthBuffer();
	~DepthBuffer();

	bool init(const glm::ivec2& dimension, DepthBufferMode mode = DepthBufferMode::RGBA, int textureCount = 1);
	void shutdown();

	bool bind();
	bool bindTexture(int textureIndex);
	void unbind();

	inline glm::ivec2 dimension() const {
		return glm::ivec2(_depthTexture.width(), _depthTexture.height());
	}

	inline Id texture() const {
		return _depthTexture.handle();
	}

	TextureType textureType() const {
		return _depthTexture.type();
	}

	inline bool depthAttachment() const {
		return _mode == DepthBufferMode::DEPTH || depthCompare();
	}

	inline bool depthCompare() const {
		return _mode == DepthBufferMode::DEPTH_CMP;
	}
private:
	GLint _oldViewport[4] = {0, 0, 0, 0};
	GLint _oldFramebuffer = -1;
	Id _fbo = video::InvalidId;
	Id _rbo = video::InvalidId;
	Texture _depthTexture;
	DepthBufferMode _mode = DepthBufferMode::RGBA;
};

inline bool bindTexture(TextureUnit unit, const DepthBuffer& depthBuffer) {
	glActiveTexture(std::enum_value(unit));
	glBindTexture(std::enum_value(depthBuffer.textureType()), depthBuffer.texture());
	return true;
}

}
