/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "core/String.h"
#include <SDL_endian.h>
#include <limits.h>
#include "core/Common.h"
#include "core/StandardLib.h"
#include "core/Assert.h"
#include "core/collection/Buffer.h"
#include "io/Stream.h"

namespace io {

class BufferedReadWriteStream : public SeekableReadStream, public WriteStream {
private:
	typedef core::Buffer<uint8_t> Buffer;
	Buffer _buffer;
	int64_t _pos = 0;
public:
	BufferedReadWriteStream(int size = 0);

	// get the raw data pointer for the buffer
	const uint8_t* getBuffer() const;
	void resize(size_t size);
	// clear the buffer if it's no longer needed
	void clear();
	int write(const void *buf, size_t size) override;
	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t pos() const override;
	int64_t size() const override;
};

}
