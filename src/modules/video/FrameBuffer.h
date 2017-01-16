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
	Id _fbo = video::InvalidId;
	Id _texture = video::InvalidId;
	Id _depth = video::InvalidId;

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

	Id texture() const;

	inline const glm::ivec2& dimension() const {
		return _dimension;
	}
};

inline Id FrameBuffer::texture() const {
	return _texture;
}
}
