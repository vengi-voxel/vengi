/**
 * @file
 */

#pragma once

#include "GLFunc.h"

namespace video {

class DepthBuffer {
public:
	DepthBuffer(bool depthAttachment = false);
	~DepthBuffer();

	bool init(const glm::ivec2& dimension);
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
		return _depthAttachment;
	}

private:
	GLint _viewport[4] = {0, 0, 0, 0};
	GLint _oldFramebuffer = -1;
	GLuint _fbo = 0u;
	GLuint _rbo = 0u;
	GLuint _depthTexture = 0u;
	glm::ivec2 _dimension;
	bool _depthAttachment;
};

}
