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

namespace io {

#define BYTE_MASK 0XFF
#define WORD_MASK 0XFFFF

class BufferedReadWriteStream {
private:
	typedef core::Buffer<uint8_t> Buffer;
	Buffer _buffer;
	int64_t _pos = 0;
public:
	BufferedReadWriteStream(int size = 0);

	void addByte(uint8_t byte);
	void addInt(int32_t dword);

	uint8_t readByte();
	int32_t readInt();

	// get the raw data pointer for the buffer
	const uint8_t* getBuffer() const;

	void append(const uint8_t *buf, size_t size);

	bool empty() const;

	// clear the buffer if it's no longer needed
	void clear();

	// return the amount of bytes in the buffer
	int64_t size() const;

	void resize(size_t size);
};

inline bool BufferedReadWriteStream::empty() const {
	return size() <= 0;
}

inline void BufferedReadWriteStream::resize(size_t size) {
	_buffer.resize(size);
}

inline void BufferedReadWriteStream::append(const uint8_t *buf, size_t size) {
	_buffer.reserve(_buffer.size() + size);
	_buffer.insert(_buffer.end(), buf, buf + size);
}

inline const uint8_t* BufferedReadWriteStream::getBuffer() const {
	return &_buffer[0] + _pos;
}

inline void BufferedReadWriteStream::clear() {
	_buffer.clear();
}

inline int64_t BufferedReadWriteStream::size() const {
	return (int64_t)_buffer.size() - _pos;
}

inline void BufferedReadWriteStream::addByte(uint8_t byte) {
	_buffer.push_back(byte);
}

inline void BufferedReadWriteStream::addInt(int32_t dword) {
	int32_t swappedDWord = SDL_SwapLE32(dword);
	_buffer.push_back(uint8_t(swappedDWord));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >> CHAR_BIT));
}

inline uint8_t BufferedReadWriteStream::readByte() {
	core_assert(size() > 0);
	const uint8_t byte = _buffer[_pos];
	++_pos;
	return byte;
}

inline int32_t BufferedReadWriteStream::readInt() {
	core_assert(size() >= 4);
	int32_t word;
	core_memcpy(&word, getBuffer(), 4);
	const int32_t val = SDL_SwapLE32(word);
	_pos += 4;
	return val;
}

}
