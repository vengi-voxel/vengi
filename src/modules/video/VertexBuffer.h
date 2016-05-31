/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include "core/Common.h"
#include <vector>

namespace video {

class VertexBuffer {
private:
	static constexpr int MAX_HANDLES = 4;
	GLuint _handles[MAX_HANDLES] = {GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE};
	GLenum _targets[MAX_HANDLES] = {GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE, GL_INVALID_VALUE};
	GLuint _handleIdx = 0;
	struct Attribute {
		uint32_t index;
		uint32_t bufferIndex;
		int size;
		GLenum type;
		int stride;
		intptr_t offset;
		bool normalized = false;
		uint8_t divisor = 0; // for instanced rendering
	};
	std::vector<Attribute> _attributes;
	GLuint _vao;
public:
	VertexBuffer(const void* data, GLsizeiptr size, GLenum target = GL_ARRAY_BUFFER);
	VertexBuffer();
	~VertexBuffer();

	void shutdown();

	bool addAttribute(uint32_t attributeIndex, uint32_t bufferIndex, int size, GLenum type = GL_FLOAT, bool normalized = false, int stride = 0, intptr_t offset = 0, uint8_t divisor = 0);

	bool update(int idx, const void* data, GLsizeiptr size);
	int32_t create(const void* data, GLsizeiptr size, GLenum target = GL_ARRAY_BUFFER);
	bool bind();
	void unbind();
	bool isValid(int idx) const;
	GLuint handle(int idx) const;
};

inline bool VertexBuffer::isValid(int idx) const {
	if (idx < 0) {
		return false;
	}
	if (idx >= (int)SDL_arraysize(_handles)) {
		return false;
	}
	return _handles[idx] != GL_INVALID_VALUE && _handles[idx] > 0;
}

inline GLuint VertexBuffer::handle(int idx) const {
	core_assert(idx >= 0 && idx < (int)SDL_arraysize(_handles));
	return _handles[idx];
}

}
