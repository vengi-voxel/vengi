/**
 * @file
 */

#include "MemoryStreamReadOnly.h"
#include <SDL_endian.h>
#include "core/Assert.h"
#include "core/Log.h"
#include <stdarg.h>

namespace core {

int MemoryStreamReadOnly::peekInt(uint32_t& val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::peekShort(uint16_t& val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const uint16_t swapped = SDL_SwapLE16(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::peekByte(uint8_t& val) const {
	const int retVal = peek(val);
	return retVal;
}

bool MemoryStreamReadOnly::readFormat(const char *fmt, ...) {
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

bool MemoryStreamReadOnly::readString(int length, char *strbuff, bool terminated) {
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

bool MemoryStreamReadOnly::readLine(int length, char *strbuff) {
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

int MemoryStreamReadOnly::readByte(uint8_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
	}
	return retVal;
}

int MemoryStreamReadOnly::readShort(uint16_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const int16_t swapped = SDL_SwapLE16(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::readShortBE(uint16_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const int16_t swapped = SDL_SwapBE16(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::readFloat(float& val) {
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

int MemoryStreamReadOnly::readFloatBE(float& val) {
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

int MemoryStreamReadOnly::readInt(uint32_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::readIntBE(uint32_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint32_t swapped = SDL_SwapBE32(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::readBuf(uint8_t *buf, size_t bufSize) {
	for (size_t i = 0; i < bufSize; ++i) {
		if (readByte(buf[i]) != 0) {
			return -1;
		}
	}
	return 0;
}

int MemoryStreamReadOnly::readLong(uint64_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint64_t swapped = SDL_SwapLE64(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::readLongBE(uint64_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint64_t swapped = SDL_SwapBE64(val);
		val = swapped;
	}
	return retVal;
}

int MemoryStreamReadOnly::seek(int64_t position) {
	if (position > _size || position < 0) {
		return -1;
	}
	_pos = position;
	return 0;
}

}
