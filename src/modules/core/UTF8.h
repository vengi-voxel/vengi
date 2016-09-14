/**
 * @file
 */

#pragma once

namespace core {
namespace utf8 {

/**
 * @brief Extract the next utf8 char from the given stream and advances the stream pointer by the char length
 * @return -1 on end of string
 */
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
