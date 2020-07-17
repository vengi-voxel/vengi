/**
 * @file
 */

#include "IParser.h"
#include <SDL_stdinc.h>
#include <stdarg.h>

namespace backend {

core::String IParser::getBetween(const core::String& str, const core::String& tokenStart, const core::String& tokenEnd) {
	const size_t start = str.find(tokenStart);
	if (start == core::String::npos) {
		return "";
	}

	const size_t end = str.find(tokenEnd);
	if (end == core::String::npos) {
		setError("syntax error - expected %s", tokenEnd.c_str());
		return "";
	}
	const size_t startIndex = start + 1;
	const size_t endIndex = end - startIndex;
	if (endIndex <= 0) {
		return "";
	}
	const core::String& between = str.substr(startIndex, endIndex);
	return between;
}

void IParser::setError(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[1024];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	_error = buf;
}

}
