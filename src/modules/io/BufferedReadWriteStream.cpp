/**
 * @file
 */

#include "BufferedReadWriteStream.h"

namespace io {

BufferedReadWriteStream::BufferedReadWriteStream(io::ReadStream &stream, int64_t size) {
	resizeBuffer(size);
	const int64_t n = stream.read(_buffer, size);
	if (n == -1) {
		return;
	}
	_size = n;
}

BufferedReadWriteStream::BufferedReadWriteStream(int64_t size) {
	resizeBuffer(size);
}

BufferedReadWriteStream::~BufferedReadWriteStream() {
	core_free(_buffer);
}

void BufferedReadWriteStream::reset() {
	_size = 0u;
	_pos = 0u;
}

uint8_t* BufferedReadWriteStream::release() {
	uint8_t *b = _buffer;
	_buffer = nullptr;
	_size = 0u;
	_capacity = 0;
	_pos = 0u;
	return b;
}

void BufferedReadWriteStream::resizeBuffer(int64_t size) {
	if (size == 0u) {
		return;
	}
	if (_capacity >= size) {
		return;
	}
	_capacity = align(size);
	if (!_buffer) {
		_buffer = (uint8_t*)core_malloc(_capacity);
	} else {
		_buffer = (uint8_t*)core_realloc(_buffer, _capacity);
	}
}

int BufferedReadWriteStream::write(const void *buf, size_t size) {
	if (size == 0) {
		return 0;
	}
	resizeBuffer(_pos + (int64_t)size);
	core_memcpy(&_buffer[_pos], buf, size);
	_pos += (int64_t)size;
	_size = core_max(_pos, _size);
	return (int)size;
}

int BufferedReadWriteStream::read(void *buf, size_t size) {
	const int64_t remainingSize = remaining();
	if (remainingSize <= 0) {
		return -1;
	}
	if ((int64_t)size > remainingSize) {
		core_memcpy(buf, &_buffer[_pos], remainingSize);
		_pos += (int64_t)remainingSize;
		return (int)remainingSize;
	}
	core_memcpy(buf, &_buffer[_pos], size);
	_pos += (int64_t)size;
	return (int)size;
}

int64_t BufferedReadWriteStream::seek(int64_t position, int whence) {
	const int64_t s = size();
	int64_t newPos = -1;
	switch (whence) {
	case SEEK_SET:
		newPos = position;
		break;
	case SEEK_CUR:
		newPos = _pos + position;
		break;
	case SEEK_END:
		newPos = s + position;
		break;
	default:
		return -1;
	}
	if (newPos < 0) {
		newPos = 0;
	} else if (newPos > s) {
		newPos = s;
	}
	_pos = newPos;
	return 0;
}

int64_t BufferedReadWriteStream::pos() const {
	return _pos;
}

int64_t BufferedReadWriteStream::size() const {
	return _size;
}

} // namespace io
