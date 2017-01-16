#include "UniformBuffer.h"

namespace video {

UniformBuffer::~UniformBuffer() {
	core_assert_msg(_handle == video::InvalidId, "Uniform buffer was not properly shut down");
	shutdown();
}

void UniformBuffer::shutdown() {
	video::deleteBuffer(_handle);
}

void UniformBuffer::create(size_t size, const void *data) {
	if (_handle != video::InvalidId) {
		shutdown();
	}
	_handle = video::genBuffer();
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
	video::bindBufferBase(VertexBufferType::UniformBuffer, _handle);
	return true;
}

}
