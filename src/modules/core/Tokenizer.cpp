#include "Tokenizer.h"

namespace core {

Tokenizer::Tokenizer(const std::string& string, const char *sep) :
		_posIndex(0u) {
	const char *s = string.c_str();

	bool lastCharIsSep = false;
	for (;;) {
		char c = skip(&s);
		if (c == '\0') {
			if (lastCharIsSep) {
				_tokens.push_back("");
			}
			break;
		}
		lastCharIsSep = false;
		std::string token;
		if (c == '"') {
			++s;
			for (;;) {
				c = *s++;
				if (c == '"' || c == '\0') {
					_tokens.push_back(token);
					break;
				} else if (c == '\\') {
					const char next = *s;
					if (next == 'n') {
						c = '\n';
					} else if (next == 't') {
						c = '\t';
					} else if (next == '"') {
						c = '"';
					}
					++s;
				}
				token.push_back(c);
			}
			continue;
		}

		lastCharIsSep = isSeparator(c, sep);
		if (lastCharIsSep) {
			_tokens.push_back("");
			++s;
			continue;
		}
		token.push_back(c);
		for (;;) {
			++s;
			c = *s;
			if (c < ' ') {
				break;
			}
			lastCharIsSep = isSeparator(c, sep);
			if (lastCharIsSep) {
				++s;
				break;
			}
			token.push_back(c);
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

char Tokenizer::skip(const char **s) const {
	char c;
	while ((c = **s) <= ' ') {
		if (c == '\0') {
			return '\0';
		}
		*s += core::string::getUTF8LengthForCharacter(c);
	}

	// skip multiline and singleline comments
	if (c == '/') {
		const char next = (*s)[1];
		if (next == '*') {
			int l = 0;
			*s += 2;
			const char* data = *s;
			while (data[l] != '\0' && data[l] != '*' && data[l + 1] != '\0' && data[l + 1] != '/') {
				++l;
			}
			*s += l + 2;
			return skip(s);
		} else if (next == '/') {
			while (**s != '\0' && **s != '\n') {
				(*s)++;
			}
			return skip(s);
		}
	}

	return c;
}

}
