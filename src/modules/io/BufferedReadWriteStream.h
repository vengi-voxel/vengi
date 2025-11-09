/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "io/Stream.h"

namespace io {

/**
 * @see SeekableReadStream
 * @see SeekableWriteStream
 * @see MemoryReadStream
 * @ingroup IO
 */
class BufferedReadWriteStream : public SeekableReadWriteStream {
private:
	uint8_t* _buffer = nullptr;
	int64_t _pos = 0;
	int64_t _capacity = 0;
	int64_t _size = 0;

	// TODO: PERF: make this configurable
	static constexpr int64_t INCREASE = 32;
	static inline constexpr int64_t align(int64_t val) {
		const uint64_t len = INCREASE - 1u;
		return (int64_t)(((uint64_t)val + len) & ~len);
	}

	void resizeBuffer(int64_t size);
public:
	BufferedReadWriteStream(io::ReadStream &stream, int64_t size);
	BufferedReadWriteStream(io::ReadStream &stream);
	BufferedReadWriteStream(int64_t size = 0);
	virtual ~BufferedReadWriteStream();

	BufferedReadWriteStream(BufferedReadWriteStream&& other) noexcept;
	BufferedReadWriteStream& operator=(BufferedReadWriteStream&& other) noexcept;

	BufferedReadWriteStream(const BufferedReadWriteStream&) = delete;
	BufferedReadWriteStream& operator=(const BufferedReadWriteStream&) = delete;

	void reserve(int bytes) override;
	void reset();

	// get the raw data pointer for the buffer
	const uint8_t* getBuffer() const;
	// return the allocated buffer - any other read or write operation on this stream is undefined
	uint8_t* release();
	int write(const void *buf, size_t size) override;
	int read(void *buf, size_t size) override;
	/**
	 * @brief Remove the first few already read bytes from the stream by moving the memory
	 */
	void trim();
	/**
	 * @param[in] position This is the number of bytes to offset
	 * @param[in] whence @c SEEK_SET offset is used as absolute position from the beginning of the stream.
	 * @c SEEK_CUR offset is taken as relative offset from the current position.
	 * @c SEEK_END offset is used relative to the end of the stream.
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t pos() const override;
	int64_t size() const override;
	int64_t capacity() const;
	bool eos() const override {
		return _pos >= _size;
	}
};

inline int64_t BufferedReadWriteStream::capacity() const {
	return _capacity;
}

inline const uint8_t* BufferedReadWriteStream::getBuffer() const {
	return _buffer;
}

}
