#pragma once

#include "UniformBuffer.h"

namespace video {

UniformBuffer::UniformBuffer() :
		_handle(0u) {
}

UniformBuffer::~UniformBuffer() {
	shutdown();
}

void UniformBuffer::shutdown() {
	glDeleteBuffers(1, &_handle);
	_handle = 0u;
}

void UniformBuffer::create(GLsizeiptr size, const void *data) {
	glGenBuffers(1, &_handle);
	glBindBuffer(GL_UNIFORM_BUFFER, _handle);
	glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

/**
 * @param[in] index The index of the uniform block to bind the buffer to
 */
void UniformBuffer::bind(GLuint index) {
	core_assert(_handle != 0u);
	// Bind the buffer object to the uniform block.
	glBindBufferBase(GL_UNIFORM_BUFFER, index, _handle);
}

}
