/**
 * @file
 */

#include "Stream.h"
#include "core/Assert.h"
#include <SDL_endian.h>
#include <SDL_stdinc.h>
#include <fcntl.h>
#include <stdio.h>

namespace io {

bool WriteStream::writeStringFormat(bool terminate, const char *fmt, ...) {
	va_list ap;
	const size_t bufSize = 4096;
	char text[bufSize];

	va_start(ap, fmt);
	SDL_vsnprintf(text, bufSize, fmt, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);
	const size_t length = SDL_strlen(text);
	for (size_t i = 0u; i < length; i++) {
		if (!writeUInt8(uint8_t(text[i]))) {
			return false;
		}
	}
	if (!terminate) {
		return true;
	}
	return writeUInt8(uint8_t('\0'));
}

bool WriteStream::writeFormat(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b':
			if (!writeUInt8((uint8_t)va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 's':
			if (!writeUInt16((uint16_t)va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 'i':
			if (!writeUInt32((uint32_t)va_arg(ap, int))) {
				va_end(ap);
				return false;
			}
			break;
		case 'l':
			if (!writeUInt64((uint64_t)va_arg(ap, long))) {
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

bool WriteStream::writeInt8(int8_t val) {
	return write(&val, sizeof(val)) != -1;
}

bool WriteStream::writeUInt8(uint8_t val) {
	return write(&val, sizeof(val)) != -1;
}

bool WriteStream::writeString(const core::String &string, bool terminate) {
	const size_t length = string.size();
	for (size_t i = 0u; i < length; i++) {
		if (!writeUInt8(uint8_t(string[i]))) {
			return false;
		}
	}
	if (!terminate) {
		return true;
	}
	return writeUInt8(uint8_t('\0'));
}

bool WriteStream::writePascalStringUInt16LE(const core::String &str) {
	uint16_t length = str.size();
	if (!writeUInt16(length)) {
		return false;
	}
	for (uint16_t i = 0u; i < length; ++i) {
		uint8_t chr = str[i];
		if (!writeUInt8(chr)) {
			return false;
		}
	}
	return true;
}

bool WriteStream::writePascalStringUInt16BE(const core::String &str) {
	uint16_t length = str.size();
	if (!writeUInt16BE(length)) {
		return false;
	}
	for (uint16_t i = 0u; i < length; ++i) {
		uint8_t chr = str[i];
		if (!writeUInt8(chr)) {
			return false;
		}
	}
	return true;
}

bool WriteStream::writePascalStringUInt32LE(const core::String &str) {
	uint16_t length = str.size();
	if (!writeUInt32(length)) {
		return false;
	}
	for (uint16_t i = 0u; i < length; ++i) {
		uint8_t chr = str[i];
		if (!writeUInt8(chr)) {
			return false;
		}
	}
	return true;
}

bool WriteStream::writePascalStringUInt32BE(const core::String &str) {
	uint16_t length = str.size();
	if (!writeUInt32BE(length)) {
		return false;
	}
	for (uint16_t i = 0u; i < length; ++i) {
		uint8_t chr = str[i];
		if (!writeUInt8(chr)) {
			return false;
		}
	}
	return true;
}

bool WriteStream::writeInt16(int16_t word) {
	const int16_t swappedWord = SDL_SwapLE16(word);
	return write(&swappedWord, sizeof(int16_t)) != -1;
}

bool WriteStream::writeInt32(int32_t dword) {
	const int32_t swappedDWord = SDL_SwapLE32(dword);
	return write(&swappedDWord, sizeof(int32_t)) != -1;
}

bool WriteStream::writeInt64(int64_t dword) {
	const int64_t swappedDWord = SDL_SwapLE64(dword);
	return write(&swappedDWord, sizeof(int64_t)) != -1;
}

bool WriteStream::writeUInt16(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapLE16(word);
	return write(&swappedWord, sizeof(uint16_t)) != -1;
}

bool WriteStream::writeUInt32(uint32_t dword) {
	const uint32_t swappedDWord = SDL_SwapLE32(dword);
	return write(&swappedDWord, sizeof(uint32_t)) != -1;
}

bool WriteStream::writeUInt64(uint64_t dword) {
	const uint64_t swappedDWord = SDL_SwapLE64(dword);
	return write(&swappedDWord, sizeof(uint64_t)) != -1;
}

bool WriteStream::writeFloat(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return writeUInt32(tmp.i);
}

bool WriteStream::writeInt16BE(int16_t word) {
	const int16_t swappedWord = SDL_SwapBE16(word);
	return write(&swappedWord, sizeof(int16_t)) != -1;
}

bool WriteStream::writeInt32BE(int32_t dword) {
	const int32_t swappedDWord = SDL_SwapBE32(dword);
	return write(&swappedDWord, sizeof(int32_t)) != -1;
}

bool WriteStream::writeInt64BE(int64_t dword) {
	const int64_t swappedDWord = SDL_SwapBE64(dword);
	return write(&swappedDWord, sizeof(int64_t)) != -1;
}

bool WriteStream::writeUInt16BE(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapBE16(word);
	return write(&swappedWord, sizeof(uint16_t)) != -1;
}

bool WriteStream::writeUInt32BE(uint32_t dword) {
	const uint32_t swappedDWord = SDL_SwapBE32(dword);
	return write(&swappedDWord, sizeof(uint32_t)) != -1;
}

bool WriteStream::writeUInt64BE(uint64_t dword) {
	const uint64_t swappedDWord = SDL_SwapBE64(dword);
	return write(&swappedDWord, sizeof(uint64_t)) != -1;
}

bool WriteStream::writeFloatBE(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return writeUInt32BE(tmp.i);
}

bool WriteStream::writeBool(bool value) {
	return writeUInt8(value);
}

bool ReadStream::readFormat(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	while (*fmt != '\0') {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b': {
			uint8_t val;
			if (readUInt8(val) != 0) {
				va_end(ap);
				return false;
			}
			*(uint8_t*)va_arg(ap, int *) = val;
			break;
		}
		case 's': {
			uint16_t val;
			if (readUInt16(val) != 0) {
				va_end(ap);
				return false;
			}
			*(uint16_t*)va_arg(ap, int *) = val;
			break;
		}
		case 'i': {
			uint32_t val;
			if (readUInt32(val) != 0) {
				va_end(ap);
				return false;
			}
			*(uint32_t*)va_arg(ap, int *) = (int)val;
			break;
		}
		case 'l': {
			uint64_t val;
			if (readUInt64(val) != 0) {
				va_end(ap);
				return false;
			}
			*(uint64_t*)va_arg(ap, int64_t *) = (int64_t)val;
			break;
		}
		default:
			core_assert_always(false);
		}
	}

	va_end(ap);
	return true;
}

bool ReadStream::readString(int length, char *strbuff, bool terminated) {
	for (int i = 0; i < length; ++i) {
		uint8_t chr;
		if (readUInt8(chr) != 0) {
			return false;
		}
		strbuff[i] = (char)chr;
		if (terminated && chr == '\0') {
			break;
		}
	}
	return true;
}

bool ReadStream::readString(int length, core::String &str, bool terminated) {
	str.clear();
	for (int i = 0; i < length; ++i) {
		uint8_t chr;
		if (readUInt8(chr) != 0) {
			return false;
		}
		str += (char)chr;
		if (terminated && chr == '\0') {
			break;
		}
	}
	return true;
}

bool ReadStream::readPascalStringUInt8(core::String &str) {
	uint8_t length;
	if (readUInt8(length) != 0) {
		return false;
	}
	return readString(length, str, false);
}

bool ReadStream::readPascalStringUInt16LE(core::String &str) {
	uint16_t length;
	if (readUInt16(length) != 0) {
		return false;
	}
	return readString(length, str, false);
}

bool ReadStream::readPascalStringUInt16BE(core::String &str) {
	uint16_t length;
	if (readUInt16BE(length) != 0) {
		return false;
	}
	return readString(length, str, false);
}

bool ReadStream::readPascalStringUInt32LE(core::String &str) {
	uint32_t length;
	if (readUInt32(length) != 0) {
		return false;
	}
	return readString((int)length, str, false);
}

bool ReadStream::readPascalStringUInt32BE(core::String &str) {
	uint32_t length;
	if (readUInt32BE(length) != 0) {
		return false;
	}
	return readString((int)length, str, false);
}

int ReadStream::readUInt8(uint8_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		return 0;
	}
	return -1;
}

int ReadStream::readInt8(int8_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		return 0;
	}
	return -1;
}

bool ReadStream::readBool() {
	uint8_t boolean;
	if (readUInt8(boolean) != 0) {
		return false;
	}
	return boolean != 0;
}

int ReadStream::readUInt16(uint16_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const uint16_t swapped = SDL_SwapLE16(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readInt16(int16_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const int16_t swapped = SDL_SwapLE16(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readInt16BE(int16_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const int16_t swapped = SDL_SwapBE16(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readUInt16BE(uint16_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const int16_t swapped = SDL_SwapBE16(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readFloat(float &val) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	const int retVal = readUInt32(tmp.i);
	if (retVal == 0) {
		val = tmp.f;
	}
	return retVal;
}

int ReadStream::readFloatBE(float &val) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	const int retVal = readUInt32BE(tmp.i);
	if (retVal == 0) {
		val = tmp.f;
	}
	return retVal;
}

int ReadStream::readUInt32(uint32_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readInt32(int32_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const int32_t swapped = (int32_t)SDL_SwapLE32(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readUInt32BE(uint32_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const uint32_t swapped = SDL_SwapBE32(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readInt32BE(int32_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const int32_t swapped = SDL_SwapBE32(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readUInt64(uint64_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const uint64_t swapped = SDL_SwapLE64(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readInt64(int64_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const int64_t swapped = SDL_SwapLE64(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readUInt64BE(uint64_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const uint64_t swapped = SDL_SwapBE64(val);
		val = swapped;
		return 0;
	}
	return -1;
}

int ReadStream::readInt64BE(int64_t &val) {
	if (read(&val, sizeof(val)) == sizeof(val)) {
		const int64_t swapped = SDL_SwapBE64(val);
		val = swapped;
		return 0;
	}
	return -1;
}

bool SeekableReadStream::readLine(int length, char *strbuff) {
	for (int i = 0; i < length; ++i) {
		uint8_t chr;
		if (readUInt8(chr) != 0) {
			return false;
		}
		if (chr == '\r') {
			strbuff[i] = '\0';
			if (read(&chr, sizeof(chr)) != sizeof(chr)) {
				if (chr != '\n') {
					seek(-1, SEEK_CUR);
				}
			}
			return true;
		} else if (chr == '\n') {
			strbuff[i] = '\0';
			return true;
		}
		strbuff[i] = (char)chr;
		if (chr == '\0') {
			return true;
		}
	}
	return false;
}

int SeekableReadStream::peekUInt32(uint32_t &val) {
	const int retVal = readUInt32(val);
	if (retVal == 0) {
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
		seek(-4, SEEK_CUR);
	}
	return retVal;
}

int SeekableReadStream::peekUInt16(uint16_t &val) {
	const int retVal = readUInt16(val);
	if (retVal == 0) {
		const uint16_t swapped = SDL_SwapLE16(val);
		val = swapped;
		seek(-2, SEEK_CUR);
	}
	return retVal;
}

int SeekableReadStream::peekUInt8(uint8_t &val) {
	const int retVal = readUInt8(val);
	if (retVal == 0) {
		seek(-1, SEEK_CUR);
	}
	return retVal;
}

int64_t SeekableReadStream::skip(int64_t delta) {
	return seek(delta, SEEK_CUR);
}

} // namespace io
