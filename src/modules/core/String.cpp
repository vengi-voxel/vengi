/**
 * @file
 */

#include "String.h"

namespace core {
namespace string {

std::string format(const char *msg, ...) {
	va_list ap;
	constexpr std::size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, msg);
	SDL_vsnprintf(text, bufSize, msg, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	return std::string(text);
}

static bool patternMatch(const char *pattern, const char *text);
static bool patternMatchMulti (const char* pattern, const char* text) {
	const char *p = pattern;
	const char *t = text;
	char c;

	for (;;) {
		c = *p++;
		if (c != '?' && c != '*')
			break;
		if (*t++ == '\0' && c == '?')
			return false;
	}

	if (c == '\0')
		return true;

	const int l = strlen(t);
	for (int i = 0; i < l; ++i) {
		if (*t == c && patternMatch(p - 1, t))
			return true;
		if (*t++ == '\0')
			return false;
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
			if (*t == '\0')
				return false;
			++t;
			break;
		default:
			if (c != *t++)
				return false;
			break;
		}
	}
	return *t == '\0';
}

bool matches (const std::string& pattern, const std::string& text)
{
	return patternMatch(pattern.c_str(), text.c_str());
}

}
}
