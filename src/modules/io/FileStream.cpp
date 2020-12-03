/**
 * @file
 */

#include "FileStream.h"
#include <SDL_endian.h>
#include "io/File.h"
#include "core/Assert.h"
#include "core/Log.h"
#include <stdarg.h>

namespace io {

FileStream::FileStream(File* file) :
		FileStream(file->_file) {
}

FileStream::FileStream(SDL_RWops* rwops) :
		_rwops(rwops) {
	core_assert(rwops != nullptr);
	_size = SDL_RWsize(_rwops);
}

FileStream::~FileStream() {
}

int FileStream::peekInt(uint32_t& val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::peekShort(uint16_t& val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const uint16_t swapped = SDL_SwapLE16(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::peekByte(uint8_t& val) const {
	const int retVal = peek(val);
	return retVal;
}

bool FileStream::addStringFormat(bool terminate, const char *fmt, ...) {
	va_list ap;
	const size_t bufSize = 4096;
	char text[bufSize];

	va_start(ap, fmt);
	SDL_vsnprintf(text, bufSize, fmt, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);
	const size_t length = SDL_strlen(text);
	for (size_t i = 0u; i < length; i++) {
		if (!addByte(uint8_t(text[i]))) {
			return false;
		}
	}
	if (!terminate) {
		return true;
	}
	return addByte(uint8_t('\0'));
}

bool FileStream::addFormat(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b':
			if (!addByte((uint8_t) va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 's':
			if (!addShort((uint16_t) va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 'i':
			if (!addInt((uint32_t) va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 'l':
			if (!addLong((uint64_t) va_arg(ap, long))) {
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

bool FileStream::readFormat(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	while (*fmt != '\0') {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b': {
			uint8_t val;
			if (readByte(val) != 0) {
				va_end(ap);
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 's': {
			uint16_t val;
			if (readShort(val) != 0) {
				va_end(ap);
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 'i': {
			uint32_t val;
			if (readInt(val) != 0) {
				va_end(ap);
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 'l': {
			uint64_t val;
			if (readLong(val) != 0) {
				va_end(ap);
				return false;
			}
			*va_arg(ap, int64_t *) = val;
			break;
		}
		default:
			core_assert_always(false);
		}
	}

	va_end(ap);
	return true;
}

bool FileStream::readString(int length, char *strbuff, bool terminated) {
	for (int i = 0; i < length; ++i) {
		if (_pos >= _size) {
			Log::error("Max stream length exceeded while reading string of length: %i (read: %i)", length, i);
			return false;
		}
		uint8_t chr;
		if (readByte(chr) != 0) {
			Log::error("Stream read error while reading string of length: %i (read: %i)", length, i);
			return false;
		}
		strbuff[i] = chr;
		if (terminated && chr == '\0') {
			break;
		}
	}
	return true;
}

bool FileStream::readLine(int length, char *strbuff) {
	for (int i = 0; i < length; ++i) {
		if (_pos >= _size) {
			Log::error("Max stream length exceeded while reading string of length: %i (read: %i)", length, i);
			return false;
		}
		uint8_t chr;
		if (readByte(chr) != 0) {
			Log::error("Stream read error while reading string of length: %i (read: %i)", length, i);
			return false;
		}
		if (chr == '\r') {
			strbuff[i] = '\0';
			if (peek(chr) == 0) {
				if (chr == '\n') {
					skip(1);
				}
			}
			break;
		} else if (chr == '\n') {
			strbuff[i] = '\0';
			break;
		}
		strbuff[i] = chr;
		if (chr == '\0') {
			break;
		}
	}
	return true;
}

int FileStream::readByte(uint8_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
	}
	return retVal;
}

int FileStream::readShort(uint16_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const int16_t swapped = SDL_SwapLE16(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::readShortBE(uint16_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const int16_t swapped = SDL_SwapBE16(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::readFloat(float& val) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	const int retVal = readInt(tmp.i);
	if (retVal == 0) {
		val = tmp.f;
	}
	return retVal;
}

int FileStream::readFloatBE(float& val) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	const int retVal = readIntBE(tmp.i);
	if (retVal == 0) {
		val = tmp.f;
	}
	return retVal;
}

int FileStream::readInt(uint32_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::readIntBE(uint32_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint32_t swapped = SDL_SwapBE32(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::readBuf(uint8_t *buf, size_t bufSize) {
	for (size_t i = 0; i < bufSize; ++i) {
		if (readByte(buf[i]) != 0) {
			return -1;
		}
	}
	return 0;
}

int FileStream::readLong(uint64_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint64_t swapped = SDL_SwapLE64(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::readLongBE(uint64_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint64_t swapped = SDL_SwapBE64(val);
		val = swapped;
	}
	return retVal;
}

bool FileStream::addByte(uint8_t val) {
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
	const uint8_t* b = buf;
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

bool FileStream::addString(const core::String& string, bool terminate) {
	const size_t length = string.size();
	for (size_t i = 0u; i < length; i++) {
		if (!addByte(uint8_t(string[i]))) {
			return false;
		}
	}
	if (!terminate) {
		return true;
	}
	return addByte(uint8_t('\0'));
}

bool FileStream::addShort(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapLE16(word);
	return write<uint16_t>(swappedWord);
}

bool FileStream::addInt(uint32_t dword) {
	uint32_t swappedDWord = SDL_SwapLE32(dword);
	return write<uint32_t>(swappedDWord);
}

bool FileStream::addLong(uint64_t dword) {
	uint64_t swappedDWord = SDL_SwapLE64(dword);
	return write<uint64_t>(swappedDWord);
}

bool FileStream::addFloat(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return addInt(tmp.i);
}

bool FileStream::addShortBE(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapBE16(word);
	return write<uint16_t>(swappedWord);
}

bool FileStream::addIntBE(uint32_t dword) {
	uint32_t swappedDWord = SDL_SwapBE32(dword);
	return write<uint32_t>(swappedDWord);
}

bool FileStream::addLongBE(uint64_t dword) {
	uint64_t swappedDWord = SDL_SwapBE64(dword);
	return write<uint64_t>(swappedDWord);
}

bool FileStream::addFloatBE(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return addIntBE(tmp.i);
}

int FileStream::seek(int64_t position) {
	if (position > _size || position < 0) {
		return -1;
	}
	_pos = position;
	return 0;
}

}
