#include "Tokenizer.h"

namespace core {

Tokenizer::Tokenizer(const std::string& string) :
		_posIndex(0u) {
	const char *s = string.c_str();

	for (;;) {
		char c = skip(&s);
		if (c == '\0') {
			break;
		}
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

		token.push_back(c);
		for (;;) {
			s++;
			c = *s;
			if (c <= ' ' || c == '(' || c == ')' || c == '{' || c == '}' || c == ';') {
				break;
			}
			token.push_back(c);
		}
		_tokens.push_back(token);
	}

	_size = _tokens.size();
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
