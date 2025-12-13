/**
 * @file
 */

#include "UniformBuffer.h"
#include "core/Assert.h"
#include "core/Log.h"
#if VIDEO_UNIFORM_BUFFER_HASH_COMPARE
#include "core/Hash.h"
#endif

namespace video {

UniformBuffer::~UniformBuffer() {
	core_assert_msg(_handle == video::InvalidId, "Uniform buffer was not properly shut down");
	shutdown();
}

void UniformBuffer::shutdown() {
	video::deleteBuffer(_handle);
	_size = 0;
#if VIDEO_UNIFORM_BUFFER_HASH_COMPARE
	_hash = 0u;
#endif
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
#if VIDEO_UNIFORM_BUFFER_HASH_COMPARE
	if (data != nullptr) {
		if (size > 0 && _size == size) {
			const uint32_t newHash = core::hash(data, (int)size);
			if (newHash == _hash) {
				return true;
			}
			_hash = newHash;
		} else {
			_hash = core::hash(data, (int)size);
		}
	} else {
		_hash = 0u;
	}
#endif
#ifndef __EMSCRIPTEN__
	core_assert_16byte_aligned(data);
#endif
	video::bufferData(_handle, BufferType::UniformBuffer, BufferMode::Dynamic, data, size);
	_size = size;
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
