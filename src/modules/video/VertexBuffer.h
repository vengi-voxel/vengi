/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include <vector>

namespace video {

class VertexBuffer {
private:
	GLuint _handle;
	GLenum _target;
	struct Attribute {
		uint32_t index;
		int size;
		GLenum type;
		bool normalized;
		int stride;
		intptr_t offset;
	};
	std::vector<Attribute> _attributes;
	GLuint _vao;
public:
	VertexBuffer(const void* data, GLsizeiptr size, GLenum target = GL_ARRAY_BUFFER);
	VertexBuffer();
	~VertexBuffer();

	bool addAttribute(uint32_t index, int size, GLenum type, bool normalized, int stride, intptr_t offset);

	bool create(const void* data, GLsizeiptr size, GLenum target = GL_ARRAY_BUFFER);
	bool bind();
	void unbind();
	bool isValid() const;
	GLuint handle() const;
};

inline bool VertexBuffer::isValid() const {
	return _handle != GL_INVALID_VALUE && _handle > 0;
}

inline GLuint VertexBuffer::handle() const {
	return _handle;
}

}
