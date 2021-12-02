/**
 * @file
 */

#pragma once

#include "io/Stream.h"

namespace io {

class MemoryReadStream : public SeekableReadStream {
private:
	const uint8_t *_buf;
	const int64_t _size;
	int64_t _pos = 0;

public:
	MemoryReadStream(const void *buf, uint32_t size) : _buf((const uint8_t*)buf), _size(size) {
	}
	virtual ~MemoryReadStream() {
	}

	int64_t size() const override;
	int64_t pos() const override;
	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
};

inline int64_t MemoryReadStream::size() const {
	return _size;
}

inline int64_t MemoryReadStream::pos() const {
	return _pos;
}

} // namespace io
