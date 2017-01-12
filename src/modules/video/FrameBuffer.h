/**
 * @file
 */

#pragma once

#include "Renderer.h"
#include <glm/vec2.hpp>

namespace video {

class FrameBuffer {
	friend class ScopedFrameBuffer;
private:
	GLuint _fbo = 0u;
	GLuint _texture = 0u;
	GLuint _depth = 0u;

	glm::ivec2 _dimension;

	GLint _oldFramebuffer = -1;
	GLint _viewport[4] = {0, 0, 0, 0};
public:
	FrameBuffer();
	~FrameBuffer();

	bool init(const glm::ivec2& dimension);
	void shutdown();

	void bind(bool read = false);
	void unbind();

	inline GLuint texture() const {
		return _texture;
	}

	inline const glm::ivec2& dimension() const {
		return _dimension;
	}
};

}
