/**
 * @file
 */

#pragma once

#include "io/Stream.h"

namespace io {

class MemoryReadStream : public ReadStream {
private:
	const uint8_t *_buf;

public:
	MemoryReadStream(const void *buf, uint32_t size) : _buf((const uint8_t*)buf) {
		_size = size;
	}
	virtual ~MemoryReadStream() {
	}

	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
};

} // namespace io
