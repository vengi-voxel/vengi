#include "UniformBuffer.h"

namespace video {

UniformBuffer::~UniformBuffer() {
	core_assert_msg(_handle == video::InvalidId, "Uniform buffer was not properly shut down");
	shutdown();
}

void UniformBuffer::shutdown() {
	video::deleteBuffer(_handle);
}

bool UniformBuffer::create(const void *data, size_t size) {
	if (_handle != video::InvalidId) {
		shutdown();
	}
	_handle = video::genBuffer();
	return update(data, size);
}

bool UniformBuffer::update(const void *data, size_t size) {
	if (_handle == video::InvalidId) {
		return false;
	}
	video::bufferData(_handle, BufferType::UniformBuffer, BufferMode::Dynamic, data, size);
	return true;
}

bool UniformBuffer::bind(uint32_t index) const {
	if (_handle == video::InvalidId) {
		return false;
	}
	video::bindBufferBase(BufferType::UniformBuffer, _handle, index);
	return true;
}

}
