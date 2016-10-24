/**
 * @file
 */

#include "FileStream.h"
#include <SDL_stdinc.h>
#include "io/File.h"

namespace io {

FileStream::FileStream(File* file) :
		FileStream(file->_file) {
}

FileStream::FileStream(SDL_RWops* rwops) :
		_rwops(rwops) {
	_size = SDL_RWsize(_rwops);
}

FileStream::~FileStream() {
}

int FileStream::peekInt(int32_t& val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const int32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::peekShort(int16_t& val) const {
	const int retVal = peek(val);
	if (retVal == 0) {
		const int16_t swapped = SDL_SwapLE16(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::peekByte(int8_t& val) const {
	const int retVal = peek(val);
	return retVal;
}

bool FileStream::addFormat(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);

	while (*fmt) {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b':
			if (!addByte((uint8_t) va_arg(ap, int))) {
				return false;
			}
			break;
		case 's':
			if (!addShort((uint16_t) va_arg(ap, int))) {
				return false;
			}
			break;
		case 'i':
			if (!addInt((uint32_t) va_arg(ap, int))) {
				return false;
			}
			break;
		case 'l':
			if (!addLong((uint64_t) va_arg(ap, long))) {
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
	while (*fmt) {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b': {
			int8_t val;
			if (readByte(val) != 0) {
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 's': {
			int16_t val;
			if (readShort(val) != 0) {
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 'i': {
			int32_t val;
			if (readInt(val) != 0) {
				return false;
			}
			*va_arg(ap, int *) = val;
			break;
		}
		case 'l': {
			int64_t val;
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
		int8_t chr;
		if (readByte(chr) != 0) {
			return false;
		}
		if (chr == '\0') {
			break;
		}
		strbuff += chr;
	}
	if (length > 0) {
		_pos += length;
	}
	return true;
}

int FileStream::readByte(int8_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
	}
	return retVal;
}

int FileStream::readShort(int16_t& val) {
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
		int32_t i;
	} tmp;
	const int retVal = readInt(tmp.i);
	val = tmp.f;
	return retVal;
}

int FileStream::readInt(int32_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const int32_t swapped = SDL_SwapLE32(val);
		val = swapped;
	}
	return retVal;
}

int FileStream::readLong(int64_t& val) {
	const int retVal = peek(val);
	if (retVal == 0) {
		_pos += sizeof(val);
		const int64_t swapped = SDL_SwapLE64(val);
		val = swapped;
	}
	return retVal;
}

bool FileStream::addByte(uint8_t byte) {
	return write<uint8_t>(byte);
}

bool FileStream::addString(const std::string& string) {
	const size_t length = string.length();
	for (std::size_t i = 0; i < length; i++) {
		if (!addByte(uint8_t(string[i]))) {
			return false;
		}
	}
	return addByte(uint8_t('\0'));
}

bool FileStream::addShort(int16_t word) {
	const int16_t swappedWord = SDL_SwapLE16(word);
	return write<int16_t>(swappedWord);
}

bool FileStream::addInt(int32_t dword) {
	int32_t swappedDWord = SDL_SwapLE32(dword);
	return write<int32_t>(swappedDWord);
}

bool FileStream::addLong(int64_t dword) {
	int64_t swappedDWord = SDL_SwapLE64(dword);
	return write<int64_t>(swappedDWord);
}

bool FileStream::addFloat(float value) {
	union toint {
		float f;
		uint32_t i;
	} tmp;
	tmp.f = value;
	return addInt(tmp.i);
}

}
