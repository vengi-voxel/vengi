/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <stdarg.h>
#include <SDL.h>
#include <SDL_endian.h>
#include "core/Common.h"
#include <limits.h>

namespace io {

class File;

/**
 * @brief Little endian file stream
 */
class FileStream {
private:
	int64_t _pos = 0;
	int64_t _size = 0;
	mutable SDL_RWops *_rwops;

public:
	FileStream(File* file);
	FileStream(SDL_RWops* rwops);
	virtual ~FileStream();

	inline int64_t remaining() const {
		return _size - _pos;
	}

	bool addBool(bool value);
	bool addByte(uint8_t byte);
	bool addShort(uint16_t word);
	bool addInt(uint32_t dword);
	bool addLong(uint64_t dword);
	bool addFloat(float value);
	bool addString(const std::string& string);
	bool addFormat(const char *fmt, ...);

	int seek(int64_t position);

	/**
	 * @return A value of @c 0 indicates no error
	 */
	template<class Ret>
	int peek(Ret& val) const {
		const size_t bufSize = sizeof(Ret);
		if (remaining() < (int64_t)bufSize) {
			return -1;
		}
		uint8_t buf[bufSize];
		SDL_RWseek(_rwops, _pos, RW_SEEK_SET);
		uint8_t *b = buf;

		size_t completeBytesRead = 0;
		size_t bytesRead = 1;
		while (completeBytesRead < bufSize && bytesRead != 0) {
			bytesRead = SDL_RWread(_rwops, b, 1, (bufSize - completeBytesRead));
			b += bytesRead;
			completeBytesRead += bytesRead;
		}
		if (completeBytesRead != bufSize) {
			return -1;
		}
		const Ret *word = (const Ret*) (void*) buf;
		val = *word;
		return 0;
	}

	template<class Type>
	inline bool write(Type val) {
		SDL_RWseek(_rwops, _pos, RW_SEEK_SET);
		const size_t bufSize = sizeof(Type);
		uint8_t buf[bufSize];
		for (size_t i = 0; i < bufSize; ++i) {
			buf[i] = uint8_t(val >> (i * CHAR_BIT));
		}

		uint8_t *b = buf;
		size_t completeBytesWritten = 0;
		int32_t bytesWritten = 1;
		while (completeBytesWritten < bufSize && bytesWritten > 0) {
			bytesWritten = SDL_RWwrite(_rwops, b, 1, (bufSize - completeBytesWritten));
			b += bytesWritten;
			completeBytesWritten += bytesWritten;
		}
		if (completeBytesWritten != bufSize) {
			return false;
		}
		if (_pos >= _size) {
			_size += sizeof(val);
		}
		_pos += sizeof(val);
		return true;
	}

	template<class Ret>
	inline int read(Ret& val) {
		const int retVal = peek<Ret>(val);
		if (retVal == 0) {
			_pos += sizeof(val);
		}
		return retVal;
	}

	int readBuf(uint8_t *buf, size_t bufSize);

	bool readBool();
	int readByte(uint8_t& val);
	int readShort(uint16_t& val);
	int readInt(uint32_t& val);
	int readLong(uint64_t& val);
	int readFloat(float& val);
	/**
	 * @brief Read a fixed-width string from a file. It may be null-terminated, but
	 * the position of the stream is still advanced by the given length
	 * @param[in] length The fixed length of the string in the file and the min length
	 * of the output buffer.
	 * @param[out] strbuff The output buffer
	 */
	bool readString(int length, char *strbuff);
	bool readFormat(const char *fmt, ...);

	int peekInt(uint32_t& val) const;
	int peekShort(uint16_t& val) const;
	int peekByte(uint8_t& val) const;

	void append(const uint8_t *buf, size_t size);

	bool empty() const;

	int64_t skip(int64_t delta);

	// return the amount of bytes in the buffer
	int64_t size() const;

	int64_t pos() const;

	FileStream &operator<<(const uint8_t &x) {
		addByte(x);
		return *this;
	}

	FileStream &operator<<(const int16_t &x) {
		addShort(x);
		return *this;
	}

	FileStream &operator<<(const bool &x) {
		addBool(x);
		return *this;
	}

	FileStream &operator<<(const int32_t &x) {
		addInt(x);
		return *this;
	}

	FileStream &operator<<(const float &x) {
		addFloat(x);
		return *this;
	}

	FileStream &operator<<(const std::string &x) {
		addString(x);
		return *this;
	}
};

inline bool FileStream::empty() const {
	return _size <= 0;
}

inline void FileStream::append(const uint8_t *buf, size_t size) {
	// TODO: optimize
	for (std::size_t i = 0; i < size; ++i) {
		addByte(buf[i]);
	}
}

inline bool FileStream::addBool(bool value) {
	return addByte(value);
}

inline bool FileStream::readBool() {
	uint8_t boolean;
	if (readByte(boolean) != 0) {
		return false;
	}
	return boolean != 0;
}

inline int64_t FileStream::skip(int64_t delta) {
	_pos += delta;
	if (_pos >= _size) {
		_pos = _size;
	} else if (_pos < 0) {
		_pos = 0;
	}
	return _pos;
}

inline int64_t FileStream::size() const {
	return _size;
}

inline int64_t FileStream::pos() const {
	return _pos;
}

}
