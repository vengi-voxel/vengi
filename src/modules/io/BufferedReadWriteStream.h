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

class BufferedReadWriteStream : public SeekableReadStream, public SeekableWriteStream {
private:
	uint8_t* _buffer = nullptr;
	int64_t _pos = 0;
	int64_t _capacity = 0;
	int64_t _size = 0;

	static constexpr int64_t INCREASE = 32;
	static inline constexpr int64_t align(int64_t val) {
		const uint64_t len = INCREASE - 1u;
		return (int64_t)(((uint64_t)val + len) & ~len);
	}

	void resizeBuffer(int64_t size);
public:
	BufferedReadWriteStream(int64_t size = 0);
	virtual ~BufferedReadWriteStream();

	// get the raw data pointer for the buffer
	const uint8_t* getBuffer() const;
	int write(const void *buf, size_t size) override;
	int read(void *buf, size_t size) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t pos() const override;
	int64_t size() const override;
	int64_t capacity() const;
};

inline int64_t BufferedReadWriteStream::capacity() const {
	return _capacity;
}

inline const uint8_t* BufferedReadWriteStream::getBuffer() const {
	return _buffer;
}

}
