/**
 * @file
 */

#pragma once

#include "GLFunc.h"

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

	bool init(const glm::ivec2& dimension, DepthBufferMode mode = DepthBufferMode::RGBA);
	void shutdown();

	void bind();
	void unbind();
	uint8_t *read();

	inline const glm::ivec2& dimension() const {
		return _dimension;
	}

	inline GLuint getTexture() const {
		return _depthTexture;
	}

	inline bool depthAttachment() const {
		return _mode == DepthBufferMode::DEPTH || _mode == DepthBufferMode::DEPTH_CMP;
	}

private:
	GLint _viewport[4] = {0, 0, 0, 0};
	GLint _oldFramebuffer = -1;
	GLuint _fbo = 0u;
	GLuint _rbo = 0u;
	GLuint _depthTexture = 0u;
	glm::ivec2 _dimension;
	DepthBufferMode _mode = DepthBufferMode::RGBA;
};

}
