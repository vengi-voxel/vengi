/**
 * @file
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace core {
namespace utf8 {

/**
 * @brief Extract the next utf8 char from the given stream and advances the stream pointer by the char length
 * @return -1 on end of string
 */
extern int next(const char** str);

extern int toUtf8(unsigned int c, char *buf, size_t bufSize);

extern size_t length(const char* str);

extern size_t lengthChar(uint8_t c);

extern size_t lengthInt(int c);

inline bool isMultibyte(char c) {
	return (c & 0xc0) == 0x80;
}

}
}
