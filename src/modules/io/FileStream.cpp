/**
 * @file
 */

#include "FileStream.h"
#include "SDL_rwops.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "io/File.h"
#include <SDL_endian.h>
#include <stdio.h>
#include <stdarg.h>

namespace io {

FileStream::FileStream(File *file) : FileStream(file->_file) {
}

FileStream::FileStream(SDL_RWops *rwops) : _rwops(rwops) {
	core_assert(rwops != nullptr);
	_size = SDL_RWsize(_rwops);
}

FileStream::~FileStream() {
}

int FileStream::peekInt(uint32_t &val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::peekShort(uint16_t &val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const uint16_t swapped = SDL_SwapLE16(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::peekByte(uint8_t &val) const {
	const int retVal = peek(val);
	return retVal;
}

bool FileStream::writeStringFormat(bool terminate, const char *fmt, ...) {
	va_list ap;
	const size_t bufSize = 4096;
	char text[bufSize];

	va_start(ap, fmt);
	SDL_vsnprintf(text, bufSize, fmt, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);
	const size_t length = SDL_strlen(text);
	for (size_t i = 0u; i < length; i++) {
		if (!writeByte(uint8_t(text[i]))) {
			return false;
		}
	}
	if (!terminate) {
		return true;
	}
	return writeByte(uint8_t('\0'));
}

bool FileStream::writeFormat(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b':
			if (!writeByte((uint8_t)va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 's':
			if (!writeShort((uint16_t)va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 'i':
			if (!writeInt((uint32_t)va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 'l':
			if (!writeLong((uint64_t)va_arg(ap, long))) {
				va_end(ap);
				return false;
			}
			break;
		default:
			core_assert_always(false);
		}
	}

	va_end(ap);
	return true;
}

bool FileStream::writeByte(uint8_t val) {
	SDL_RWseek(_rwops, _pos, RW_SEEK_SET);
	if (SDL_RWwrite(_rwops, &val, 1, 1) != 1) {
		return false;
	}
	if (_pos >= _size) {
		++_size;
	}
	++_pos;
	return true;
}

bool FileStream::append(const uint8_t *buf, size_t size) {
	SDL_RWseek(_rwops, _pos, RW_SEEK_SET);
	size_t completeBytesWritten = 0;
	int32_t bytesWritten = 1;
	const uint8_t *b = buf;
	while (completeBytesWritten < size && bytesWritten > 0) {
		bytesWritten = (int32_t)SDL_RWwrite(_rwops, b, 1, (size - completeBytesWritten));
		b += bytesWritten;
		completeBytesWritten += bytesWritten;
	}
	if (completeBytesWritten != size) {
		return false;
	}
	if (_pos >= _size) {
		_size += size;
	}
	_pos += size;
	return true;
}

bool FileStream::writeString(const core::String &string, bool terminate) {
	const size_t length = string.size();
	for (size_t i = 0u; i < length; i++) {
		if (!writeByte(uint8_t(string[i]))) {
			return false;
		}
	}
	if (!terminate) {
		return true;
	}
	return writeByte(uint8_t('\0'));
}

bool FileStream::writeShort(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapLE16(word);
	return write<uint16_t>(swappedWord);
}

bool FileStream::writeInt(uint32_t dword) {
	uint32_t swappedDWord = SDL_SwapLE32(dword);
	return write<uint32_t>(swappedDWord);
}

bool FileStream::writeLong(uint64_t dword) {
	uint64_t swappedDWord = SDL_SwapLE64(dword);
	return write<uint64_t>(swappedDWord);
}

bool FileStream::writeFloat(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return writeInt(tmp.i);
}

bool FileStream::writeShortBE(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapBE16(word);
	return write<uint16_t>(swappedWord);
}

bool FileStream::writeIntBE(uint32_t dword) {
	uint32_t swappedDWord = SDL_SwapBE32(dword);
	return write<uint32_t>(swappedDWord);
}

bool FileStream::writeLongBE(uint64_t dword) {
	uint64_t swappedDWord = SDL_SwapBE64(dword);
	return write<uint64_t>(swappedDWord);
}

bool FileStream::writeFloatBE(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return writeIntBE(tmp.i);
}

int FileStream::read(void *dataPtr, size_t dataSize) {
	if (remaining() < (int64_t)dataSize) {
		return -1;
	}
	uint8_t *b = (uint8_t*)dataPtr;
	size_t completeBytesRead = 0;
	size_t bytesRead = 1;
	while (completeBytesRead < dataSize && bytesRead != 0) {
		bytesRead = SDL_RWread(_rwops, b, 1, (dataSize - completeBytesRead));
		b += bytesRead;
		_pos += (int64_t)bytesRead;
		completeBytesRead += bytesRead;
	}
	if (completeBytesRead != dataSize) {
		return -1;
	}
	return 0;
}

int64_t FileStream::seek(int64_t position, int whence) {
	switch (whence) {
	case SEEK_CUR:
		if (_pos + position > _size || _pos + position < 0) {
			return -1;
		}
		_pos += position;
		break;
	case SEEK_SET:
		if (position > _size || position < 0) {
			return -1;
		}
		_pos = position;
		break;
	case SEEK_END:
		if (position < 0 || _size - position < 0) {
			return -1;
		}
		_pos = _size - position;
		break;
	}
	return SDL_RWseek(_rwops, position, whence);
}

} // namespace io
