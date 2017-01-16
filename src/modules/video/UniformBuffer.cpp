#include "UniformBuffer.h"

namespace video {

UniformBuffer::UniformBuffer() {
}

UniformBuffer::~UniformBuffer() {
	core_assert_msg(_handle == video::InvalidId, "Uniform buffer was not properly shut down");
	shutdown();
}

void UniformBuffer::shutdown() {
	video::deleteBuffer(_handle);
}

void* UniformBuffer::lock(BufferLockMode mode) {
	if (_handle == video::InvalidId) {
		return nullptr;
	}
	video::bindBuffer(VertexBufferType::UniformBuffer, _handle);
	void* result = glMapBufferRange(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)_size, std::enum_value(mode));
	video::bindBuffer(VertexBufferType::UniformBuffer, InvalidId);
	return result;
}

void UniformBuffer::unlock() {
	if (_handle == video::InvalidId) {
		return;
	}
	video::bindBuffer(VertexBufferType::UniformBuffer, _handle);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	video::bindBuffer(VertexBufferType::UniformBuffer, InvalidId);
}


void UniformBuffer::create(size_t size, const void *data) {
	if (_handle != video::InvalidId) {
		shutdown();
	}
	_size = size;
	glGenBuffers(1, &_handle);
	if (data != nullptr) {
		video::bindBuffer(VertexBufferType::UniformBuffer, _handle);
		video::bufferData(VertexBufferType::UniformBuffer, VertexBufferMode::Static, data, size);
		video::bindBuffer(VertexBufferType::UniformBuffer, InvalidId);
	}
}

bool UniformBuffer::bind(uint32_t index) const {
	if (_handle == video::InvalidId) {
		return false;
	}
	// Bind the buffer object to the uniform block.
	glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint)index, _handle);
	return true;
}

}
