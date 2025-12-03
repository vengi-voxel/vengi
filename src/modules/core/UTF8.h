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

extern int toUtf16(const char *utf8, size_t utf8Size, uint16_t *utf16, size_t utf16Size);

extern int toUtf8(uint32_t c, char *buf, size_t bufSize);
/**
 * @param[in] wchars The input string
 * @param[in] wcharSize The size of the input string in characters
 * @param[out] buf The output buffer
 * @param[in] bufSize The size of the output buffer in bytes
 */
extern int toUtf8(const uint16_t *wchars, size_t wcharSize, char *buf, size_t bufSize);

extern size_t length(const char* str);

extern size_t lengthUTF16(const char* str);

extern size_t lengthChar(uint8_t c);

extern size_t lengthInt(int c);

inline bool isMultibyte(char c) {
	return (c & 0xc0) == 0x80;
}

}
}
