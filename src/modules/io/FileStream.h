/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "io/Stream.h"
#include <SDL_rwops.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

namespace io {

class File;
typedef core::SharedPtr<File> FilePtr;

/**
 * @brief Little endian file stream
 */
class FileStream : public ReadStream {
private:
	int64_t _pos = 0;
	int64_t _size = 0;
	mutable SDL_RWops *_rwops;

public:
	FileStream(File *file);
	FileStream(const FilePtr &file) : FileStream(file.get()) {
	}
	FileStream(SDL_RWops *rwops);
	virtual ~FileStream();

	inline int64_t remaining() const {
		return _size - _pos;
	}

	bool writeBool(bool value);
	bool writeByte(uint8_t val);
	bool writeShort(uint16_t word);
	bool writeInt(uint32_t dword);
	bool writeLong(uint64_t dword);
	bool writeFloat(float value);
	bool writeShortBE(uint16_t word);
	bool writeIntBE(uint32_t dword);
	bool writeLongBE(uint64_t dword);
	bool writeFloatBE(float value);
	bool writeStringFormat(bool terminate, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(3);
	bool writeString(const core::String &string, bool terminate = true);
	bool writeFormat(const char *fmt, ...);

	int read(void *dataPtr, size_t dataSize) override;

	/**
	 * @return -1 on error
	 */
	int64_t seek(int64_t position, int whence = SEEK_SET) override;

	/**
	 * @return A value of @c 0 indicates no error
	 * @note This does not handle the endianness
	 */
	template <class Ret> int peek(Ret &val) const {
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
			SDL_RWseek(_rwops, _pos, RW_SEEK_SET);
			return -1;
		}
		SDL_RWseek(_rwops, _pos, RW_SEEK_SET);
		const Ret *word = (const Ret *)(void *)buf;
		val = *word;
		return 0;
	}

	template <class Type> inline bool write(Type val) {
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
			bytesWritten = (int32_t)SDL_RWwrite(_rwops, b, 1, (bufSize - completeBytesWritten));
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

	int peekInt(uint32_t &val) const;
	int peekShort(uint16_t &val) const;
	int peekIntBE(uint32_t &val) const;
	int peekShortBE(uint16_t &val) const;
	int peekByte(uint8_t &val) const;

	bool append(const uint8_t *buf, size_t size);

	bool empty() const;

	int64_t skip(int64_t delta);

	// return the amount of bytes in the buffer
	int64_t size() const;

	int64_t pos() const;
};

inline bool FileStream::empty() const {
	return _size <= 0;
}

inline bool FileStream::writeBool(bool value) {
	return writeByte(value);
}

inline int64_t FileStream::skip(int64_t delta) {
	_pos += delta;
	if (_pos >= _size) {
		_pos = _size;
	} else if (_pos < 0) {
		_pos = 0;
	}
	SDL_RWseek(_rwops, _pos, RW_SEEK_SET);
	return _pos;
}

inline int64_t FileStream::size() const {
	return _size;
}

inline int64_t FileStream::pos() const {
	return _pos;
}

} // namespace io
