/**
 * @file
 */

#pragma once

#include "io/Stream.h"

namespace io {

/**
 * @ingroup IO
 * @see SeekableReadStream
 * @see BufferedReadWriteStream
 */
class MemoryReadStream : public SeekableReadStream {
protected:
	const uint8_t *_buf = nullptr;
	uint8_t *_ownBuf = nullptr;
	int64_t _size;
	int64_t _pos = 0;

public:
	MemoryReadStream(const void *buf, uint32_t size);
	MemoryReadStream(ReadStream &stream, uint32_t size);
	virtual ~MemoryReadStream();

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
