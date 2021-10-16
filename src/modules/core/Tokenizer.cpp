/**
 * @file
 */

#include "Tokenizer.h"
#include "core/UTF8.h"
#include <string.h>

namespace core {

Tokenizer::Tokenizer(bool skipComments_, const char* s, size_t len, const char *sep, const char *split) :
		_posIndex(0u), _len((int32_t)len), _skipComments(skipComments_) {
	bool lastCharIsSep = false;
	bool lastCharWasQuoteEnd = false;
	for (;;) {
		char c = skip(&s);
		if (c == '\0') {
			if (lastCharIsSep) {
				_tokens.push_back("");
			}
			break;
		}
		lastCharIsSep = false;
		core::String token;
		// TODO: check that " is not sep or split
quote:
		if (c == '"') {
			size_t cl = core::utf8::lengthChar(c);
			if (cl == 0u) {
				return;
			}
			_len -= cl;
			s += cl;
			for (;;) {
				// don't skip comments or whitespaces here, an inner string should be preserved
				c = *s;
				cl = core::utf8::lengthChar(c);
				if (cl == 0u) {
					return;
				}
				_len -= cl;
				s += cl;
				if (c == '"') {
					lastCharWasQuoteEnd = true;
					c = *s;
					break;
				}
				if (c == '\0' || _len <= 0) {
					lastCharWasQuoteEnd = true;
					break;
				}
				if (c == '\\') {
					const char next = *s;
					if (next == 'n') {
						c = '\n';
					} else if (next == 't') {
						c = '\t';
					} else if (next == '"') {
						c = '"';
					}
					++s;
					--_len;
				}
				token += c;
			}
		}
		if (lastCharWasQuoteEnd) {
			lastCharWasQuoteEnd = false;
			if (c < ' ' || _len <= 0) {
				_tokens.push_back(token);
				if (_len <= 0) {
					break;
				}
				continue;
			}
		}

		lastCharIsSep = isSeparator(c, sep);
		if (lastCharIsSep) {
			_tokens.push_back(token);
			const size_t cl = core::utf8::lengthChar(c);
			if (cl == 0u) {
				return;
			}
			_len -= cl;
			s += cl;
			continue;
		}
		token += c;
		if (isSeparator(c, split)) {
			_tokens.push_back(token);
			const size_t cl = core::utf8::lengthChar(c);
			if (cl == 0u) {
				return;
			}
			_len -= cl;
			s += cl;
			continue;
		}
		for (;;) {
			size_t cl = core::utf8::lengthChar(c);
			if (cl == 0u) {
				return;
			}
			_len -= cl;
			s += cl;
			if (skipComments(&s, false)) {
				break;
			}
			c = skip(&s, false);
			if (c < ' ' || _len <= 0) {
				break;
			}
			if (c == '"') {
				goto quote;
			}
			lastCharIsSep = isSeparator(c, sep);
			if (lastCharIsSep) {
				cl = core::utf8::lengthChar(c);
				if (cl == 0u) {
					return;
				}
				_len -= cl;
				s += cl;
				break;
			}
			if (isSeparator(c, split)) {
				_tokens.push_back(token);
				token = "";
				token += c;
				_tokens.push_back(token);
				token = "";
				continue;
			}
			token += c;
		}
		_tokens.push_back(token);
	}

	_size = _tokens.size();
}

bool Tokenizer::isSeparator(char c, const char *sep) {
	const char *sepPtr = sep;
	while (*sepPtr != '\0') {
		if (c == *sepPtr) {
			return true;
		}
		++sepPtr;
	}
	return false;
}

bool Tokenizer::skipComments(const char **s, bool skipWhitespace) {
	if (!_skipComments) {
		return false;
	}
	const char c = **s;
	if (c != '/' && c != '#') {
		return false;
	}
	const char next = (*s)[1];
	if (next == '*') {
		int l = 0;
		_len -= 2;
		if (_len < 0) {
			return false;
		}
		*s += 2;
		const char* data = *s;
		while (data[l] != '\0' && data[l] != '*' && data[l + 1] != '\0' && data[l + 1] != '/') {
			++l;
		}
		_len -= l + 2;
		if (_len < 0) {
			return false;
		}
		*s += l + 2;
		skip(s, skipWhitespace);
		return true;
	} else if (next == '/' || c == '#') {
		while (**s != '\0' && **s != '\n') {
			(*s)++;
			if (--_len <= 0) {
				return true;
			}
		}
		skip(s, skipWhitespace);
		return true;
	}
	return false;
}

char Tokenizer::skip(const char **s, bool skipWhitespace) {
	if (_len <= 0) {
		return '\0';
	}
	char c = **s;
	if (skipWhitespace) {
		while ((c = **s) <= ' ') {
			if (c == '\0' || _len <= 0) {
				return '\0';
			}
			const size_t cl = core::utf8::lengthChar(c);
			if (cl == 0u) {
				return '\0';
			}
			_len -= cl;
			if (_len < 0) {
				return '\0';
			}
			*s += cl;
		}
	}

	if (!skipComments(s, skipWhitespace)) {
		const size_t cl = core::utf8::lengthChar(c);
		if (cl == 0u) {
			return '\0';
		}
		return c;
	}
	return **s;
}

}
