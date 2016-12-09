#include "UniformBuffer.h"

namespace video {

UniformBuffer::UniformBuffer() :
		_handle(0u) {
}

UniformBuffer::~UniformBuffer() {
	core_assert_msg(_handle == 0u, "Uniform buffer was not properly shut down");
	shutdown();
}

void UniformBuffer::shutdown() {
	glDeleteBuffers(1, &_handle);
	_handle = 0u;
}

void* UniformBuffer::lock(BufferLockMode mode) {
	if (_handle == 0) {
		return nullptr;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, _handle);
	void* result = glMapBufferRange(GL_UNIFORM_BUFFER, 0, _size, std::enum_value(mode));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return result;
}

void UniformBuffer::unlock() {
	if (_handle == 0u) {
		return;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, _handle);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void UniformBuffer::create(GLsizeiptr size, const void *data) {
	if (_handle != 0u) {
		shutdown();
	}
	_size = size;
	glGenBuffers(1, &_handle);
	if (data != nullptr) {
		glBindBuffer(GL_UNIFORM_BUFFER, _handle);
		glBufferData(GL_UNIFORM_BUFFER, _size, data, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

/**
 * @param[in] index The index of the uniform block to bind the buffer to
 */
bool UniformBuffer::bind(GLuint index) const {
	if (_handle == 0u) {
		return false;
	}
	// Bind the buffer object to the uniform block.
	glBindBufferBase(GL_UNIFORM_BUFFER, index, _handle);
	return true;
}

}
