/**
 * @file
 */

#pragma once

#include "GLFunc.h"
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

	inline GLuint texture() const {
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
	GLuint _fbo = 0u;
	GLuint _rbo = 0u;
	Texture _depthTexture;
	DepthBufferMode _mode = DepthBufferMode::RGBA;
};

}
