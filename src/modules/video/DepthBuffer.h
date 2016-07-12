/**
 * @file
 */

#pragma once

#include "GLFunc.h"

namespace video {

class DepthBuffer {
public:
	DepthBuffer();
	~DepthBuffer();

	bool init(const glm::ivec2& dimension);
	void shutdown();

	void bind();
	void unbind();
	uint8_t *read();

	inline GLuint getTexture() const {
		return _depthTexture;
	}

private:
	GLint _viewport[4] = {0, 0, 0, 0};
	GLint _oldFramebuffer = -1;
	GLuint _fbo = 0u;
	GLuint _rbo = 0u;
	GLuint _depthTexture = 0u;
	glm::ivec2 _dimension;
};

}
