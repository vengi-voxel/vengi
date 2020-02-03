/**
 * @file
 */

#include "StringUtil.h"
#include "Common.h"
#include <ctype.h>

namespace core {
namespace string {

char* getBeforeToken(char **buffer, const char *token, size_t bufferSize) {
	char *begin = *buffer;
	const size_t length = SDL_strlen(token);
	while (**buffer) {
		if (bufferSize <= 0) {
			return nullptr;
		}
		if (SDL_strncmp(*buffer, token, length) == 0) {
			**buffer = '\0';
			*buffer += length;
			return begin;
		}
		++(*buffer);
		--bufferSize;
	}
	*buffer = begin;
	return nullptr;
}

bool formatBuf(char *buf, size_t bufSize, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	const bool fit = SDL_vsnprintf(buf, bufSize, msg, ap) < (int)bufSize;
	buf[bufSize - 1] = '\0';
	va_end(ap);
	return fit;
}

core::String format(const char *msg, ...) {
	va_list ap;
	constexpr std::size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, msg);
	SDL_vsnprintf(text, bufSize, msg, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	return core::String(text);
}

char *urlEncode(const char *inBuf) {
	const char *inBufPos = inBuf;
	const size_t maxSize = strlen(inBuf) * 3;
	char *outBuf = (char*)core_malloc(maxSize + 1);
	char *outBufPos = outBuf;
	while (inBufPos[0] != '\0') {
		const char inChr = inBufPos[0];
		if (inChr == ' ') {
			*outBufPos++ = '+';
		} else if (inChr == '-' || inChr == '.' || inChr == '~' || inChr == '_' || isalnum(inChr)) {
			*outBufPos++ = inChr;
		} else {
			*outBufPos++ = '%';
			*outBufPos++ = toHex(inChr >> 4);
			*outBufPos++ = toHex(inChr & 15);
		}
		++inBufPos;
	}
	*outBufPos = '\0';
	return outBuf;
}

void replaceAllChars(core::String& str, char in, char out) {
	char *p = (char*)str.c_str();
	while (*p != '\0') {
		if (*p == in) {
			*p = out;
		}
		++p;
	}
}

core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr, size_t replaceStrSize) {
	if (str.empty()) {
		return str;
	}
	core::String sNew = str;
	size_t loc;
	const size_t searchLength = searchStr.size();
	size_t lastPosition = 0;
	while (core::String::npos != (loc = sNew.find(searchStr, lastPosition))) {
		sNew.replace(loc, searchLength, replaceStr);
		lastPosition = loc + replaceStrSize;
	}
	return sNew;
}

void splitString(const core::String& string, std::vector<core::String>& tokens, const char* delimiters) {
	// Skip delimiters at beginning.
	size_t lastPos = string.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	size_t pos = string.find_first_of(delimiters, lastPos);

	while (core::String::npos != pos || core::String::npos != lastPos) {
		// Found a token, add it to the vector.
		tokens.push_back(string.substr(lastPos, pos - lastPos));
		// Skip delimiters. Note the "not_of"
		lastPos = string.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = string.find_first_of(delimiters, lastPos);
	}
}

bool isNumber(const core::String &in) {
	char *end = nullptr;
	double val = strtod(in.c_str(), &end);
	return end != in.c_str() && *end == '\0' && val != HUGE_VAL;
}

bool isInteger(const core::String& in) {
	for (size_t i = 0u; i < in.size(); i++) {
		if (!isdigit(in[i])) {
			return false;
		}
	}
	return true;
}

core::String toLower(const core::String& string) {
	core::String convert = string;
	std::transform(convert.begin(), convert.end(), convert.begin(), (int (*)(int)) SDL_tolower);
	return convert;
}

core::String toLower(const char* string) {
	core::String convert(string);
	std::transform(convert.begin(), convert.end(), convert.begin(), (int (*)(int)) SDL_tolower);
	return convert;
}

core::String toUpper(const core::String& string) {
	core::String convert = string;
	std::transform(convert.begin(), convert.end(), convert.begin(), (int (*)(int)) SDL_toupper);
	return convert;
}

core::String toUpper(const char* string) {
	core::String convert(string);
	std::transform(convert.begin(), convert.end(), convert.begin(), (int (*)(int)) SDL_toupper);
	return convert;
}

static bool patternMatch(const char *pattern, const char *text);
static bool patternMatchMulti(const char* pattern, const char* text) {
	const char *p = pattern;
	const char *t = text;
	char c;

	for (;;) {
		c = *p++;
		if (c != '?' && c != '*') {
			break;
		}
		if (*t++ == '\0' && c == '?') {
			return false;
		}
	}

	if (c == '\0') {
		return true;
	}

	const int l = SDL_strlen(t);
	for (int i = 0; i < l; ++i) {
		if (*t == c && patternMatch(p - 1, t)) {
			return true;
		}
		if (*t++ == '\0') {
			return false;
		}
	}
	return false;
}

static bool patternMatch(const char *pattern, const char *text) {
	const char *p = pattern;
	const char *t = text;
	char c;

	while ((c = *p++) != '\0') {
		switch (c) {
		case '*':
			return patternMatchMulti(p, t);
		case '?':
			if (*t == '\0') {
				return false;
			}
			++t;
			break;
		default:
			if (c != *t++) {
				return false;
			}
			break;
		}
	}
	return *t == '\0';
}

bool matches(const core::String& pattern, const char* text) {
	if (pattern.empty()) {
		return true;
	}
	return patternMatch(pattern.c_str(), text);
}

bool matches(const char* pattern, const char* text) {
	if (pattern == nullptr || pattern[0] == '\0') {
		return true;
	}
	return patternMatch(pattern, text);
}

static void camelCase(core::String& str, bool upperCamelCase) {
	if (str.empty()) {
		return;
	}

	size_t startIndex = str.find_first_not_of("_", 0);
	if (startIndex == core::String::npos) {
		str = "";
		return;
	}
	if (startIndex > 0) {
		str = str.substr(startIndex);
	}
	size_t pos = str.find_first_of("_", 0);
	while (core::String::npos != pos) {
		core::String sub = str.substr(0, pos);
		core::String second = str.substr(pos + 1, str.size() - (pos + 1));
		if (!second.empty()) {
			second[0] = SDL_toupper(second[0]);
			sub.append(second);
		}
		str = sub;
		if (str.empty()) {
			return;
		}
		pos = str.find_first_of("_", pos);
	}
	if (str.empty()) {
		return;
	}
	if (!upperCamelCase) {
		str[0] = SDL_tolower(str[0]);
	} else {
		str[0] = SDL_toupper(str[0]);
	}
}

core::String lowerCamelCase(const core::String& str) {
	core::String copy = str;
	lowerCamelCase(copy);
	return copy;
}

core::String upperCamelCase(const core::String& str) {
	core::String copy = str;
	upperCamelCase(copy);
	return copy;
}

void upperCamelCase(core::String& str) {
	camelCase(str, true);
}

void lowerCamelCase(core::String& str) {
	camelCase(str, false);
}

char* append(char* buf, size_t bufsize, const char* string) {
	const size_t bufl = SDL_strlen(buf);
	if (bufl >= bufsize) {
		return nullptr;
	}
	const size_t remaining = bufsize - bufl;
	if (remaining <= 1u) {
		return nullptr;
	}
	const size_t l = SDL_strlen(string);
	if (remaining <= l) {
		return nullptr;
	}
	char* p = buf + bufl;
	for (size_t i = 0u; i < l; ++i) {
		*p++ = *string++;
	}
	*p = '\0';
	return p;
}

int count(const char *buf, char chr) {
	if (buf == nullptr) {
		return 0;
	}
	int count = 0;
	for (;;) {
		buf = strchr(buf, chr);
		if (buf == nullptr) {
			break;
		}
		++buf;
		++count;
	}
	return count;
}

}
}
