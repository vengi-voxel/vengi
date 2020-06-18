/**
 * @file
 */

#include "IndirectDrawBuffer.h"

namespace video {

bool IndirectDrawBuffer::init() {
	_handle = video::genBuffer();
	if (_handle == video::InvalidId) {
		return false;
	}
	return true;
}

void IndirectDrawBuffer::shutdown() {
	deleteBuffer(_handle);
}

bool IndirectDrawBuffer::update(const void *data, size_t size) {
	if (_handle == video::InvalidId) {
		return false;
	}

	video::bufferData(_handle, video::BufferType::IndirectBuffer, video::BufferMode::Dynamic, data, size);
	return true;
}

void IndirectDrawBuffer::bind() const {
	video::bindBuffer(video::BufferType::IndirectBuffer, _handle);
}

void IndirectDrawBuffer::unbind() const {
	video::unbindBuffer(video::BufferType::IndirectBuffer);
}

}