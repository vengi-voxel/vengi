/**
 * @file
 */

#include "PersistentMappingBuffer.h"
#include "Renderer.h"

namespace video {

PersistentMappingBuffer::PersistentMappingBuffer(size_t size) :
		_size(size) {
}

bool PersistentMappingBuffer::init() {
	_handle = video::genBuffer();
	if (_handle == video::InvalidId) {
		return false;
	}
	if (!video::bindBuffer(video::BufferType::ArrayBuffer, _handle)) {
		return false;
	}

	_memory = video::bufferStorage(video::BufferType::ArrayBuffer, _size);
	if (_memory == nullptr) {
		return false;
	}

	return true;
}

void PersistentMappingBuffer::shutdown() {
	//video::unmapBuffer(_handle, video::BufferType::ArrayBuffer);
	deleteBuffer(_handle);
	_memory = nullptr;
}

bool PersistentMappingBuffer::write(const uint8_t *data, size_t offset, size_t size) {
	if (_memory == nullptr) {
		return false;
	}
	core_memcpy(_memory + (intptr_t)offset, data, size);
	_lockMgr.lockRange(offset, size);
	return true;
}

bool PersistentMappingBuffer::wait(size_t offset, size_t size) {
	_lockMgr.waitForLockedRange(offset, size);
	return true;
}

}
