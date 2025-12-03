/**
 * @file
 */

#include "UTF8.h"
#include "core/StandardLib.h"

namespace core {
namespace utf8 {

int toUtf8(const uint16_t *wchars, size_t wcharSize, char *buf, size_t bufSize) {
	if (wcharSize == 0) {
		if (bufSize <= 0) {
			return -1;
		}
		buf[0] = '\0';
		return 0;
	}
	size_t bufPos = 0;
	uint32_t w;
	bool leadSubst = false;
	for (size_t i = 0; i < wcharSize; ++i) {
		const uint16_t wchar = wchars[i];
		if (wchar >= 0xd800 && wchar <= 0xdbff) {
			if (!leadSubst) {
				leadSubst = true;
				w = 0x010000 + ((wchar & 0x3ff) << 10);
			} else {
				leadSubst = false;
				w = 0;
			}
		} else if (wchar >= 0xdc00 && wchar <= 0xdfff) {
			if (leadSubst) {
				if (bufPos + 4 >= bufSize) {
					return -1;
				}
				leadSubst = false;
				w = w | (wchar & 0x3ff);
				buf[bufPos++] = (char)(0xf0 + ((w >> 18) & 0x7));
				buf[bufPos++] = (char)(0x80 + ((w >> 12) & 0x3f));
				buf[bufPos++] = (char)(0x80 + ((w >> 6) & 0x3f));
				buf[bufPos++] = (char)(0x80 + (w & 0x3f));
			}
		} else if (wchar >= 0x800) {
			if (bufPos + 3 >= bufSize) {
				return -1;
			}
			buf[bufPos++] = (char)(0xe0 + ((wchar >> 12) & 0xf));
			buf[bufPos++] = (char)(0x80 + ((wchar >> 6) & 0x3f));
			buf[bufPos++] = (char)(0x80 + (wchar & 0x3f));
		} else if (wchar >= 0x80) {
			if (bufPos + 2 >= bufSize) {
				return -1;
			}
			buf[bufPos++] = (char)(0xc0 + ((wchar >> 6) & 0x1f));
			buf[bufPos++] = (char)(0x80 + (wchar & 0x3f));
		} else {
			if (bufPos + 1 >= bufSize) {
				return -1;
			}
			buf[bufPos++] = (char)(wchar & 0x7f);
		}
	}
	buf[bufPos] = '\0';
	return (int)bufPos;
}

int toUtf8(uint32_t c, char *buf, size_t bufSize) {
	if (c < 0x80) {
		if (bufSize < 1) {
			return 0;
		}
		buf[0] = (char)c;
		return 1;
	}

	if (c < 0x800) {
		if (bufSize < 2) {
			return 0;
		}
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));
		return 2;
	}

	if (c >= 0xdc00 && c < 0xe000) {
		return 0;
	}

	if (c >= 0xd800 && c < 0xdc00) {
		if (bufSize < 4) {
			return 0;
		}
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + (c & 0x3f));
		return 4;
	}

	if (bufSize < 3) {
		return 0;
	}
	buf[0] = (char)(0xe0 + (c >> 12));
	buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
	buf[2] = (char)(0x80 + (c & 0x3f));
	return 3;
}

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

size_t length(const char *str) {
	size_t result = 0;

	while (str[0] != '\0') {
		const size_t n = lengthChar((uint8_t)*str);
		if (n == 0) {
			return 0;
		}
		str += n;
		result++;
	}
	return result;
}

size_t lengthUTF16(const char *str) {
	size_t result = 0;

	while (str[0] != '\0') {
		const size_t n = lengthChar((uint8_t)*str);
		if (n == 0) {
			return 0;
		}
		str += n;
		result += (n == 4 ? 2 : 1);
	}
	return result;
}

int next(const char **str) {
	const char *s = *str;
	if (s[0] == '\0') {
		return -1;
	}

	const unsigned char *buf = (const unsigned char *)s;
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

int toUtf16(const char *utf8, size_t utf8Size, uint16_t *utf16, size_t utf16Size) {
	size_t utf16Pos = 0;
	size_t utf8Pos = 0;
	while (utf8Pos < utf8Size) {
		const uint8_t c = utf8[utf8Pos];
		if (c < 0x80) {
			if (utf16Pos >= utf16Size) {
				return -1;
			}
			utf16[utf16Pos++] = c;
			utf8Pos++;
		} else if (c < 0xc0) {
			return -1;
		} else if (c < 0xe0) {
			if (utf8Pos + 1 >= utf8Size) {
				return -1;
			}
			if (utf16Pos + 1 >= utf16Size) {
				return -1;
			}
			utf16[utf16Pos++] = ((utf8[utf8Pos] & 0x1f) << 6) | (utf8[utf8Pos + 1] & 0x3f);
			utf8Pos += 2;
		} else if (c < 0xf0) {
			if (utf8Pos + 2 >= utf8Size) {
				return -1;
			}
			if (utf16Pos + 1 >= utf16Size) {
				return -1;
			}
			utf16[utf16Pos++] = ((utf8[utf8Pos] & 0x0f) << 12) | ((utf8[utf8Pos + 1] & 0x3f) << 6) | (utf8[utf8Pos + 2] & 0x3f);
			utf8Pos += 3;
		} else if (c < 0xf8) {
			if (utf8Pos + 3 >= utf8Size) {
				return -1;
			}
			if (utf16Pos + 1 >= utf16Size) {
				return -1;
			}
			utf16[utf16Pos++] = ((utf8[utf8Pos] & 0x07) << 18) | ((utf8[utf8Pos + 1] & 0x3f) << 12) | ((utf8[utf8Pos + 2] & 0x3f) <<
				6) | (utf8[utf8Pos + 3] & 0x3f);
			utf8Pos += 4;
		} else {
			return -1;
		}
	}
	return (int)utf16Pos;
}

} // namespace utf8
} // namespace core
