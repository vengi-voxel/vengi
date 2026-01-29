/**
 * @file
 */

#include "ShaderStorageBuffer.h"
#include "core/Assert.h"
#include "core/Log.h"

namespace video {

ShaderStorageBuffer::~ShaderStorageBuffer() {
	core_assert_msg(_handle == video::InvalidId, "Shader storage buffer was not properly shut down");
	shutdown();
}

void ShaderStorageBuffer::shutdown() {
	video::deleteBuffer(_handle);
	_size = 0;
}

bool ShaderStorageBuffer::create(const void *data, size_t size) {
	if (_handle != video::InvalidId) {
		shutdown();
	}
	_handle = video::genBuffer();
	if (_handle == video::InvalidId) {
		Log::error("Failed to create shader storage buffer");
		return false;
	}
	return update(data, size);
}

bool ShaderStorageBuffer::update(const void *data, size_t size) {
	if (_handle == video::InvalidId) {
		return false;
	}
	video::bufferData(_handle, BufferType::ShaderStorageBuffer, BufferMode::Dynamic, data, size);
	_size = size;
	return true;
}

bool ShaderStorageBuffer::update(size_t offset, const void *data, size_t size) {
	if (_handle == video::InvalidId) {
		return false;
	}
	if (offset + size > _size) {
		Log::error("ShaderStorageBuffer::update: offset + size exceeds buffer size");
		return false;
	}
	video::bufferSubData(_handle, BufferType::ShaderStorageBuffer, (intptr_t)offset, data, size);
	return true;
}

bool ShaderStorageBuffer::bind(uint32_t index) const {
	if (_handle == video::InvalidId) {
		return false;
	}
	video::bindBufferBase(BufferType::ShaderStorageBuffer, _handle, index);
	return true;
}

void *ShaderStorageBuffer::map(AccessMode mode) {
	if (_handle == video::InvalidId) {
		return nullptr;
	}
	return video::mapBuffer(_handle, BufferType::ShaderStorageBuffer, mode);
}

void ShaderStorageBuffer::unmap() {
	if (_handle == video::InvalidId) {
		return;
	}
	video::unmapBuffer(_handle, BufferType::ShaderStorageBuffer);
}

} // namespace video
