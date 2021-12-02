/**
 * @file
 */

#include "BufferedReadWriteStream.h"

namespace io {

BufferedReadWriteStream::BufferedReadWriteStream(int size) {
	_buffer.reserve(size);
}

void BufferedReadWriteStream::resize(size_t size) {
	_buffer.resize(size);
}

int BufferedReadWriteStream::write(const void *buf, size_t size) {
	_buffer.reserve(_buffer.size() + size);
	_buffer.insert(_buffer.end(), (const uint8_t *)buf, (const uint8_t *)buf + size);
	return 0;
}

int BufferedReadWriteStream::read(void *dataPtr, size_t dataSize) {
	if (_pos + dataSize > _buffer.size()) {
		return -1;
	}
	core_memcpy(dataPtr, &_buffer[_pos], dataSize);
	_pos += (int64_t)dataSize;
	return 0;
}

int64_t BufferedReadWriteStream::seek(int64_t position, int whence) {
	const int64_t s = (int64_t)_buffer.size();
	switch (whence) {
	case SEEK_SET:
		_pos = position;
		break;
	case SEEK_CUR:
		_pos += position;
		break;
	case SEEK_END:
		_pos = s - position;
		break;
	}
	if (_pos < 0) {
		_pos = 0;
	} else if (_pos > s) {
		_pos = s;
	}
	return _pos;
}

const uint8_t *BufferedReadWriteStream::getBuffer() const {
	return &_buffer[0];
}

void BufferedReadWriteStream::clear() {
	_buffer.clear();
}

int64_t BufferedReadWriteStream::pos() const {
	return _pos;
}

int64_t BufferedReadWriteStream::size() const {
	return (int64_t)_buffer.size();
}

} // namespace io
