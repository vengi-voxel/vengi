/**
 * @file
 */

#include "StringUtil.h"
#include "core/Common.h"
#include "core/ArrayLength.h"
#include "core/StandardLib.h"
#include <SDL_platform.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>

namespace core {
namespace string {

int64_t toLong(const char* str) {
	return ::atol(str);
}

int toInt(const char* str) {
	return SDL_atoi(str);
}

float toFloat(const core::String& str) {
	return (float)SDL_atof(str.c_str());
}

double toDouble(const core::String& str) {
	return (double)SDL_atof(str.c_str());
}

double toDouble(const char* str) {
	return (double)SDL_atof(str);
}

bool startsWith(const char* string, const char* token) {
	return !SDL_strncmp(string, token, SDL_strlen(token));
}

bool startsWith(const core::String& string, const char* token) {
	const size_t tokensize = SDL_strlen(token);
	if (string.size() < tokensize) {
		return false;
	}
	return !string.compare(0, tokensize, token);
}

bool contains(const char *haystack, const char *needle) {
	const char *pos = (const char*)SDL_strstr(haystack, needle);
	return pos != nullptr;
}

char toUpper(char in) { return (char)SDL_toupper((int)in); }
char toLower(char in) { return (char)SDL_tolower((int)in); }

bool iequals(const core::String& a, const core::String& b) {
	const size_t sz = a.size();
	if (b.size() != sz) {
		return false;
	}
	for (size_t i = 0u; i < sz; ++i) {
		if (toLower(a[i]) != toLower(b[i])) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Locate the string after the last occurrence of the given character in the input string
 * @return nullptr if the character is not part of the input string. Otherwise the pointer to the character
 * followed by the last found match.
 */
const char* after(const char* input, int character) {
	const char *s = SDL_strrchr(input, character);
	if (s != nullptr) {
		++s;
	}
	return s;
}

core::String toHex(int32_t number) {
	constexpr size_t hexChars = sizeof(int32_t) << 1;
	static const char *digits = "0123456789abcdef";
	core::String rc(8, '0');
	for (size_t i = 0, j = (hexChars - 1) * 4; i < hexChars; ++i, j -= 4) {
		rc[i] = digits[(number >> j) & 0x0f];
	}
	return rc;
}

core::String eraseAllChars(const core::String& str, char chr) {
	if (str.empty()) {
		return str;
	}
	core::String tmp;
	tmp.reserve(str.size() + 1);
	for (auto c : str) {
		if (c == chr) {
			continue;
		}
		tmp += c;
	}
	return tmp;
}

char* getBeforeToken(char **buffer, const char *token, size_t bufferSize) {
	if (bufferSize <= 0) {
		return nullptr;
	}
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

// TODO: take NAME_MAX into account
core::String sanitizeFilename(const core::String& input) {
	static const char unsafeChars[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
										0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
										0x1d, 0x1e, 0x1f, 0x22, 0x2a, 0x2f, 0x3a, 0x3c, 0x3e, 0x3f, 0x5c, 0x7c, 0x7f, 0x00};

	if (input.size() == 0) {
		return input;
	}

	core::String output = input;
	char *c = output.c_str();
	for (; *c; ++c) {
		if (SDL_strchr(unsafeChars, *c)) {
			*c = '_';
		}
	}
	while (output.contains("  ")) {
		output = replaceAll(output, "  ", " ");
	}
	return core::string::trim(output);
}

core::String format(const char *msg, ...) {
	va_list ap;
	const size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, msg);
	SDL_vsnprintf(text, bufSize, msg, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	return core::String(text);
}

core::String humanSize(uint64_t bytes) {
	static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
	static const char length = lengthof(units);

	int unitIdx = 0;
	double dblBytes = bytes;
	for (; bytes / 1024 > 0 && unitIdx < length - 1; ++unitIdx, bytes /= 1024) {
		dblBytes = bytes / 1024.0;
	}

	return core::string::format("%.02lf%s", dblBytes, units[unitIdx]);
}

char *urlEncode(const char *inBuf) {
	const char *inBufPos = inBuf;
	const size_t maxSize = SDL_strlen(inBuf) * 3;
	char *outBuf = (char*)core_malloc(maxSize + 1);
	char *outBufPos = outBuf;
	while (inBufPos[0] != '\0') {
		const uint8_t inChr = inBufPos[0];
		if (inChr == ' ') {
			*outBufPos++ = '+';
		} else if (inChr == '-' || inChr == '.' || inChr == '~' || inChr == '_' || isalnum(inChr)) {
			*outBufPos++ = inChr;
		} else {
			*outBufPos++ = '%';
			*outBufPos++ = toHex((char)(inChr >> 4));
			*outBufPos++ = toHex((char)(inChr & 15));
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

core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr) {
	return replaceAll(str, searchStr, replaceStr, SDL_strlen(replaceStr));
}

core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr, size_t replaceStrSize) {
	if (str.empty()) {
		return str;
	}
	if (searchStr.empty()) {
		return str;
	}

	core::String s = str;
	for (size_t pos = s.find(searchStr); pos != core::String::npos; pos = s.find(searchStr, pos + replaceStrSize)) {
		s.replace(pos, searchStr.size(), replaceStr);
	}
	return s;
}

void splitString(const core::String& string, core::DynamicArray<core::String>& tokens, const char* delimiters) {
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
	double val = SDL_strtod(in.c_str(), &end);
	return end != in.c_str() && *end == '\0' && val != HUGE_VAL;
}

bool isAlpha(int c) {
	return ::isalpha(c);
}

bool isInteger(const core::String& in) {
	for (size_t i = 0u; i < in.size(); i++) {
		if (!SDL_isdigit(in[i])) {
			if (i == 0 && in[i] == '-') {
				continue;
			}
			return false;
		}
	}
	return true;
}

bool isIntegerWithPostfix(const core::String& in) {
	for (size_t i = 0u; i < in.size(); i++) {
		if (!SDL_isdigit(in[i])) {
			if (i == 0 && in[i] == '-') {
				continue;
			}
			if (in[i] != 'u' && in[i] != 'U') {
				return false;
			}
		}
	}
	return true;
}

bool isAbsolutePath(const core::String &in) {
#ifdef __WINDOWS__
	if (in.size() >= 3 && in[0] != '\0' && isAlpha(in[0]) && in[1] == ':' && (in[2] == '\\' || in[2] == '/')) {
		return true;
	}
#endif
	return in.size() > 1U && (in[0] == '/' || in[0] == '\\');
}

static bool patternMatch(const char *text, const char *pattern);
static bool patternMatchMulti(const char* text, const char* pattern) {
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

static bool patternMatch(const char *text, const char *pattern) {
	const char *p = pattern;
	const char *t = text;
	char c;

	while ((c = *p++) != '\0') {
		switch (c) {
		case '*':
			return patternMatchMulti(t, p);
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

bool matches(const char* text, const core::String& pattern) {
	if (pattern.empty()) {
		return true;
	}
	return patternMatch(text, pattern.c_str());
}

bool matches(const char* text, const char* pattern) {
	if (pattern == nullptr || pattern[0] == '\0') {
		return true;
	}
	return patternMatch(text, pattern);
}

bool fileMatchesMultiple(const char* text, const char* patterns) {
	char buf[4096];
	SDL_strlcpy(buf, patterns, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';

	char *sep = SDL_strstr(buf, ",");
	if (sep == nullptr) {
		if (SDL_strchr(buf, '*') != nullptr) {
			return core::string::matches(text, buf);
		}
		char patternBuf[32];
		SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", buf);
		return core::string::matches(text, patternBuf);
	}

	char *f = buf;
	while (*sep == ',') {
		*sep = '\0';
		char patternBuf[32];
		if (SDL_strchr(f, '*') != nullptr) {
			SDL_strlcpy(patternBuf, f, sizeof(patternBuf));
		} else {
			SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", f);
		}
		if (core::string::matches(text, patternBuf)) {
			return true;
		}
		f = ++sep;
		sep = SDL_strchr(f, ',');
		if (sep == nullptr) {
			break;
		}
	}
	char patternBuf[32];
	if (SDL_strchr(f, '*') != nullptr) {
		SDL_strlcpy(patternBuf, f, sizeof(patternBuf));
	} else {
		SDL_snprintf(patternBuf, sizeof(patternBuf), "*.%s", f);
	}
	return core::string::matches(text, patternBuf);
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
			second[0] = toUpper(second[0]);
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
		str[0] = toLower(str[0]);
	} else {
		str[0] = toUpper(str[0]);
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
		buf = SDL_strchr(buf, chr);
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
