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
}

bool ShaderStorageBuffer::create(const void *data, size_t size, BufferMode mode) {
	if (_handle != video::InvalidId) {
		shutdown();
	}
	_handle = video::genBuffer();
	return update(data, size, mode);
}

bool ShaderStorageBuffer::update(const void *data, size_t size, BufferMode mode) {
	if (_handle == video::InvalidId) {
		return false;
	}
	const size_t maxSize = video::limit(Limit::MaxShaderStorageBufferSize);
	if (size > maxSize) {
		Log::error("Failed to update the ssbo because the max allowed size was exceeded: %u vs %u", (uint32_t)size, (uint32_t)maxSize);
		return false;
	}
	video::bufferData(_handle, BufferType::ShaderStorageBuffer, mode, data, size);
	return true;
}

bool ShaderStorageBuffer::bind(uint32_t index) const {
	if (_handle == video::InvalidId) {
		return false;
	}
	video::bindBufferBase(BufferType::ShaderStorageBuffer, _handle, index);
	return true;
}

}
