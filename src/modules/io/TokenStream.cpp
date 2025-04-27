/**
 * @file
 */

#include "io/TokenStream.h"

namespace io {

bool TokenStream::skipUntil(uint8_t &c, const char *end, core::String *content) {
	const char *s = end;
	while (*s != '\0') {
		if (_stream.readUInt8(c) == -1) {
			return false;
		}
		if (content) {
			*content += (char)c;
		}
		if ((char)c == *s) {
			++s;
		} else {
			s = end;
		}
	}
	if (_stream.eos()) {
		c = '\0';
		return true;
	}
	if (_stream.readUInt8(c) == -1) {
		return false;
	}
	return true;
}

bool TokenStream::isComment(uint8_t c) {
	if (c != '/') {
		return false;
	}
	uint8_t next;
	if (_stream.peekUInt8(next) == -1) {
		return false;
	}
	return next == '/' || next == '*';
}

bool TokenStream::skipComments(uint8_t &c) {
	if (!isComment(c)) {
		return false;
	}
	uint8_t next;
	if (_stream.readUInt8(next) == -1) {
		return false;
	}
	if (next == '*') {
		skipUntil(c, "*/");
		return true;
	} else if (next == '/') {
		skipUntil(c, "\n");
		return true;
	}
	return false;
}

bool TokenStream::skipWhitespaces(uint8_t &c) {
	for (;;) {
		if (c == '\0' || c > ' ' || _stream.eos()) {
			break;
		}
		if (_stream.readUInt8(c) == -1) {
			return false;
		}
	}
	return true;
}

bool TokenStream::nextTokenChar(const core::String &token, uint8_t &c, bool skipWhitespace) {
	if (c == '\0') {
		return false;
	}
	if (!token.empty() && (isSeparator(c) || isComment(c))) {
		_stream.seek(-1, SEEK_CUR);
		// split token here
		return false;
	}
	if (skipWhitespace) {
		// skip leading whitespaces
		skipWhitespaces(c);
	}
	if (_cfg.skipComments) {
		const bool commentsSkipped = skipComments(c);
		if (commentsSkipped) {
			nextTokenChar(token, c, skipWhitespace);
		}
	}
	// found a valid token char
	return true;
}

bool TokenStream::isSeparator(char c) const {
	if (c == '\0') {
		return true;
	}
	for (const char *s = _separator; *s != '\0'; ++s) {
		if (*s == c) {
			return true;
		}
	}
	return false;
}

core::String TokenStream::next() {
	if (_stream.eos()) {
		return core::String::Empty;
	}
	core::String token;
	while (!_stream.eos()) {
		uint8_t c;
		if (_stream.readUInt8(c) == -1) {
			break;
		}
		// TODO: this is not utf8 ready
		if (!nextTokenChar(token, c)) {
			break;
		}
		if (c == '\0') {
			break;
		}
		if (c == '"') {
			skipUntil(c, "\"", &token);
			if (!token.empty()) {
				token.erase(token.size() - 1, 1);
			}
			break;
		}
		token += (char)c;
	}
	return token;
}

} // namespace io
