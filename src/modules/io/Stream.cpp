/**
 * @file
 */

#include "Stream.h"
#include <SDL_endian.h>
#include <SDL_stdinc.h>
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
		if (!writeByte(uint8_t(text[i]))) {
			return false;
		}
	}
	if (!terminate) {
		return true;
	}
	return writeByte(uint8_t('\0'));
}

bool WriteStream::writeFormat(const char *fmt, ...) {
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

bool WriteStream::writeByte(uint8_t val) {
	return write(&val, sizeof(val)) > 0;
}

bool WriteStream::writeString(const core::String &string, bool terminate) {
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

bool WriteStream::writeShort(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapLE16(word);
	return write(&swappedWord, sizeof(uint16_t)) > 0;
}

bool WriteStream::writeInt(uint32_t dword) {
	uint32_t swappedDWord = SDL_SwapLE32(dword);
	return write(&swappedDWord, sizeof(uint32_t)) > 0;
}

bool WriteStream::writeLong(uint64_t dword) {
	uint64_t swappedDWord = SDL_SwapLE64(dword);
	return write(&swappedDWord, sizeof(uint64_t)) > 0;
}

bool WriteStream::writeFloat(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return writeInt(tmp.i);
}

bool WriteStream::writeShortBE(uint16_t word) {
	const uint16_t swappedWord = SDL_SwapBE16(word);
	return write(&swappedWord, sizeof(uint16_t)) > 0;
}

bool WriteStream::writeIntBE(uint32_t dword) {
	uint32_t swappedDWord = SDL_SwapBE32(dword);
	return write(&swappedDWord, sizeof(uint32_t)) > 0;
}

bool WriteStream::writeLongBE(uint64_t dword) {
	uint64_t swappedDWord = SDL_SwapBE64(dword);
	return write(&swappedDWord, sizeof(uint64_t)) > 0;
}

bool WriteStream::writeFloatBE(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return writeIntBE(tmp.i);
}

bool WriteStream::writeBool(bool value) {
	return writeByte(value);
}

bool ReadStream::readFormat(const char *fmt, ...) {
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
			*va_arg(ap, int *) = (int)val;
			break;
		}
		case 'l': {
			uint64_t val;
			if (readLong(val) != 0) {
				va_end(ap);
				return false;
			}
			*va_arg(ap, int64_t *) = (int64_t)val;
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
		if (readByte(chr) != 0) {
			return false;
		}
		strbuff[i] = (char)chr;
		if (terminated && chr == '\0') {
			break;
		}
	}
	return true;
}

bool ReadStream::readLine(int length, char *strbuff) {
	for (int i = 0; i < length; ++i) {
		uint8_t chr;
		if (readByte(chr) != 0) {
			return false;
		}
		if (chr == '\r') {
			strbuff[i] = '\0';
			if (read(&chr, sizeof(chr)) != 0) {
				if (chr != '\n') {
					seek(-1, SEEK_CUR);
				}
			}
			break;
		} else if (chr == '\n') {
			strbuff[i] = '\0';
			break;
		}
		strbuff[i] = (char)chr;
		if (chr == '\0') {
			break;
		}
	}
	return true;
}

int ReadStream::readByte(uint8_t &val) {
	return read(&val, sizeof(val));
}

bool ReadStream::readBool() {
	uint8_t boolean;
	if (readByte(boolean) != 0) {
		return false;
	}
	return boolean != 0;
}

int ReadStream::readShort(uint16_t &val) {
	const int retVal = read(&val, sizeof(val));
	if (retVal == 0) {
		const int16_t swapped = SDL_SwapLE16(val);
		val = swapped;
	}
	return retVal;
}

int ReadStream::readShortBE(uint16_t &val) {
	const int retVal = read(&val, sizeof(val));
	if (retVal == 0) {
		const int16_t swapped = SDL_SwapBE16(val);
		val = swapped;
	}
	return retVal;
}

int ReadStream::readFloat(float &val) {
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

int ReadStream::readFloatBE(float &val) {
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

int ReadStream::readInt(uint32_t &val) {
	const int retVal = read(&val, sizeof(val));
	if (retVal == 0) {
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int ReadStream::readIntBE(uint32_t &val) {
	const int retVal = read(&val, sizeof(val));
	if (retVal == 0) {
		const uint32_t swapped = SDL_SwapBE32(val);
		val = swapped;
	}
	return retVal;
}

int ReadStream::readLong(uint64_t &val) {
	const int retVal = read(&val, sizeof(val));
	if (retVal == 0) {
		const uint64_t swapped = SDL_SwapLE64(val);
		val = swapped;
	}
	return retVal;
}

int ReadStream::readLongBE(uint64_t &val) {
	const int retVal = read(&val, sizeof(val));
	if (retVal == 0) {
		const uint64_t swapped = SDL_SwapBE64(val);
		val = swapped;
	}
	return retVal;
}

int ReadStream::peekInt(uint32_t &val) {
	const int retVal = readInt(val);
	if (retVal == 0) {
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
		seek(-4);
	}
	return retVal;
}

int ReadStream::peekShort(uint16_t &val) {
	const int retVal = readShort(val);
	if (retVal == 0) {
		const uint16_t swapped = SDL_SwapLE16(val);
		val = swapped;
		seek(-2);
	}
	return retVal;
}

int ReadStream::peekByte(uint8_t &val) {
	const int retVal = readByte(val);
	if (retVal == 0) {
		seek(-1);
	}
	return retVal;
}

} // namespace io
