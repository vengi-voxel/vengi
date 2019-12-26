/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>
#include <stdarg.h>
#include <SDL_endian.h>
#include <limits.h>
#include "Common.h"
#include "Assert.h"

namespace core {

#define BYTE_MASK 0XFF
#define WORD_MASK 0XFFFF

class ByteStream {
private:
	typedef std::vector<uint8_t> VectorBuffer;
	VectorBuffer _buffer;
	int _pos;

	inline int size() const {
		return (int)(_buffer.size() - _pos);
	}

	inline VectorBuffer::const_iterator begin() const {
		return _buffer.begin() + _pos;
	}

	inline VectorBuffer::iterator begin() {
		return _buffer.begin() + _pos;
	}

public:
	ByteStream(int size = 0);

	void addBool(bool value, bool prepend = false);
	void addByte(uint8_t byte, bool prepend = false);
	void addShort(int16_t word, bool prepend = false);
	void addInt(int32_t dword);
	void addLong(int64_t dword);
	void addFloat(float value);
	void addString(const std::string& string);
	void addFormat(const char *fmt, ...);

	bool readBool();
	uint8_t readByte();
	int16_t readShort();
	int32_t readInt();
	int64_t readLong();
	float readFloat();
	std::string readString();
	void readFormat(const char *fmt, ...);

	int32_t peekInt() const;
	int16_t peekShort() const;

	// get the raw data pointer for the buffer
	const uint8_t* getBuffer() const;

	void append(const uint8_t *buf, size_t size);

	bool empty() const;

	// clear the buffer if it's no longer needed
	void clear();

	// return the amount of bytes in the buffer
	size_t getSize() const;

	void resize(size_t size);

	ByteStream &operator<<(const uint8_t &x) {
		addByte(x, false);
		return *this;
	}

	ByteStream &operator<<(const int16_t &x) {
		addShort(x);
		return *this;
	}

	ByteStream &operator<<(const bool &x) {
		addBool(x);
		return *this;
	}

	ByteStream &operator<<(const int32_t &x) {
		addInt(x);
		return *this;
	}

	ByteStream &operator<<(const float &x) {
		addFloat(x);
		return *this;
	}

	ByteStream &operator<<(const std::string &x) {
		addString(x);
		return *this;
	}
};

inline bool ByteStream::empty() const {
	return size() <= 0;
}

inline void ByteStream::resize(size_t size) {
	_buffer.resize(size);
}

inline void ByteStream::append(const uint8_t *buf, size_t size) {
	_buffer.reserve(_buffer.size() + size);
	_buffer.insert(_buffer.end(), buf, buf + size);
}

inline const uint8_t* ByteStream::getBuffer() const {
	return &_buffer[0] + _pos;
}

inline void ByteStream::clear() {
	_buffer.clear();
}

inline size_t ByteStream::getSize() const {
	return _buffer.size() - _pos;
}

inline void ByteStream::addByte(uint8_t byte, bool prepend) {
	if (prepend) {
		_buffer.insert(_buffer.begin(), byte);
	} else {
		_buffer.push_back(byte);
	}
}

inline void ByteStream::addBool(bool value, bool prepend) {
	addByte(value, prepend);
}

inline void ByteStream::addString(const std::string& string) {
	const size_t length = string.length();
	for (std::size_t i = 0; i < length; i++) {
		addByte(uint8_t(string[i]));
	}
	addByte(uint8_t('\0'));
}

inline void ByteStream::addShort(int16_t word, bool prepend) {
	const int16_t swappedWord = SDL_SwapLE16(word);
	if (prepend) {
		_buffer.insert(_buffer.begin(), uint8_t(swappedWord >> CHAR_BIT));
		_buffer.insert(_buffer.begin(), uint8_t(swappedWord));
	} else {
		_buffer.push_back(uint8_t(swappedWord));
		_buffer.push_back(uint8_t(swappedWord >> CHAR_BIT));
	}
}

inline void ByteStream::addInt(int32_t dword) {
	int32_t swappedDWord = SDL_SwapLE32(dword);
	_buffer.push_back(uint8_t(swappedDWord));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >> CHAR_BIT));
}

inline void ByteStream::addLong(int64_t dword) {
	int64_t swappedDWord = SDL_SwapLE64(dword);
	_buffer.push_back(uint8_t(swappedDWord));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	_buffer.push_back(uint8_t(swappedDWord >> CHAR_BIT));
}

inline void ByteStream::addFloat(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	addInt(tmp.i);
}

inline uint8_t ByteStream::readByte() {
	core_assert(size() > 0);
	const uint8_t byte = _buffer[_pos];
	++_pos;
	return byte;
}

inline bool ByteStream::readBool() {
	return readByte() != 0;
}

inline int16_t ByteStream::readShort() {
	core_assert(size() >= 2);
	const int16_t *word = reinterpret_cast<const int16_t*>(getBuffer());
	const int16_t val = SDL_SwapLE16(*word);
	_pos += 2;
	return val;
}

inline float ByteStream::readFloat() {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.i = readInt();

	return tmp.f;
}

inline int32_t ByteStream::readInt() {
	core_assert(size() >= 4);
	const int32_t *word = reinterpret_cast<const int32_t*>(getBuffer());
	const int32_t val = SDL_SwapLE32(*word);
	_pos += 4;
	return val;
}

inline int64_t ByteStream::readLong() {
	core_assert(size() >= 8);
	const int64_t *word = reinterpret_cast<const int64_t*>(getBuffer());
	const int64_t val = SDL_SwapLE64(*word);
	_pos += 8;
	return val;
}

}
