/**
 * @file
 */

#include "FileStream.h"
#include <SDL_stdinc.h>
#include "io/File.h"
#include "core/Assert.h"

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
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 's': {
			uint16_t val;
			if (readShort(val) != 0) {
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 'i': {
			uint32_t val;
			if (readInt(val) != 0) {
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 'l': {
			uint64_t val;
			if (readLong(val) != 0) {
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

bool FileStream::readString(int length, char *strbuff) {
	for (int i = 0; i < length; ++i) {
		if (_pos >= _size) {
			return false;
		}
		uint8_t chr;
		if (readByte(chr) != 0) {
			return false;
		}
		strbuff[i] = chr;
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

int FileStream::readInt(uint32_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const uint32_t swapped = SDL_SwapLE32(val);
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

bool FileStream::addByte(uint8_t byte) {
	return write<uint8_t>(byte);
}

bool FileStream::addString(const std::string& string, bool terminate) {
	const size_t length = string.length();
	for (std::size_t i = 0; i < length; i++) {
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

int FileStream::seek(int64_t position) {
	if (position > _size || position < 0) {
		return -1;
	}
	_pos = position;
	return 0;
}

}
