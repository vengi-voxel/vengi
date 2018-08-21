/**
 * @file
 */

#include "UTF8.h"

namespace core {
namespace utf8 {

size_t lengthChar(uint8_t c) {
	if (c < 0x80) {
		return 1;
	}
	if (c < 0xc0) {
		return 0;
	}
	if (c < 0xe0) {
		return 2;
	}
	if (c < 0xf0) {
		return 3;
	}
	if (c < 0xf8) {
		return 4;
	}
	/* 5 and 6 byte sequences are no longer valid. */
	return 0;
}

size_t lengthInt(int c) {
	if (c <= 0x7F) {
		return 1;
	}
	if (c <= 0x07FF) {
		return 2;
	}
	if (c <= 0xFFFF) {
		return 3;
	}
	if (c <= 0x10FFFF) { /* highest defined Unicode code */
		return 4;
	}
	return 0;
}

size_t length(const char* str) {
	size_t result = 0;

	while (str[0] != '\0') {
		const size_t n = lengthChar((uint8_t) *str);
		str += n;
		result++;
	}
	return result;
}

int next(const char** str) {
	const char* s = *str;
	if (s[0] == '\0') {
		return -1;
	}

	const unsigned char* buf = (const unsigned char*)(s);
	size_t len;
	int cp;
	int min;

	if (buf[0] < 0x80) {
		len = 1;
		min = 0;
		cp = buf[0];
	} else if (buf[0] < 0xC0) {
		return -1;
	} else if (buf[0] < 0xE0) {
		len = 2;
		min = 1 << 7;
		cp = buf[0] & 0x1F;
	} else if (buf[0] < 0xF0) {
		len = 3;
		min = 1 << (5 + 6);
		cp = buf[0] & 0x0F;
	} else if (buf[0] < 0xF8) {
		len = 4;
		min = 1 << (4 + 6 + 6);
		cp = buf[0] & 0x07;
	} else {
		return -1;
	}

	for (size_t i = 1; i < len; i++) {
		if ((buf[i] & 0xc0) != 0x80) {
			return -1;
		}
		cp = (cp << 6) | (buf[i] & 0x3F);
	}

	if (cp < min) {
		return -1;
	}

	if (0xD800 <= cp && cp <= 0xDFFF) {
		return -1;
	}

	if (0x110000 <= cp) {
		return -1;
	}

	*str += len;
	return cp;
}
}

}
