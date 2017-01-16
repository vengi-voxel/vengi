#include "UniformBuffer.h"

namespace video {

UniformBuffer::UniformBuffer() {
}

UniformBuffer::~UniformBuffer() {
	core_assert_msg(_handle == video::InvalidId, "Uniform buffer was not properly shut down");
	shutdown();
}

void UniformBuffer::shutdown() {
	if (_handle != video::InvalidId) {
		glDeleteBuffers(1, &_handle);
		_handle = video::InvalidId;
	}
}

void* UniformBuffer::lock(BufferLockMode mode) {
	if (_handle == video::InvalidId) {
		return nullptr;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, _handle);
	void* result = glMapBufferRange(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)_size, std::enum_value(mode));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return result;
}

void UniformBuffer::unlock() {
	if (_handle == video::InvalidId) {
		return;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, _handle);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void UniformBuffer::create(size_t size, const void *data) {
	if (_handle != video::InvalidId) {
		shutdown();
	}
	_size = size;
	glGenBuffers(1, &_handle);
	if (data != nullptr) {
		glBindBuffer(GL_UNIFORM_BUFFER, _handle);
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)_size, data, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

/**
 * @param[in] index The index of the uniform block to bind the buffer to
 */
bool UniformBuffer::bind(GLuint index) const {
	if (_handle == video::InvalidId) {
		return false;
	}
	// Bind the buffer object to the uniform block.
	glBindBufferBase(GL_UNIFORM_BUFFER, index, _handle);
	return true;
}

}
