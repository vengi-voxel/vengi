/**
 * @file
 */

#pragma once

#include "Common.h"
#include "String.h"

namespace core {

class Tokenizer {
protected:
	std::vector<std::string> _tokens;
	std::size_t _posIndex;
	std::size_t _size;

	inline char skip(const char **s) const {
		char c;
		while ((c = **s) <= ' ') {
			if (c == '\0')
				return '\0';
			*s += core::string::getUTF8LengthForCharacter(c);
		}

		// skip multiline and singleline comments
		if (c == '/') {
			const char next = (*s)[1];
			if (next == '*') {
				int l = 0;
				*s += 2;
				const char* data = *s;
				while (!(data[l] != '\0' && data[l] == '*') && (data[l + 1] != '\0' && data[l + 1] == '/')) {
					++l;
				}
				*s += l + 2;
			} else if (next == '/') {
				while (**s != '\0' && **s != '\n') {
					(*s)++;
				}
			}
		}

		return c;
	}
public:
	Tokenizer(const std::string& string) :
			_posIndex(0u) {
		const char *s = string.c_str();

		for (;;) {
			char c = skip(&s);
			if (c == '\0')
				break;
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
				if (c <= ' ' || c == '(' || c == ')' || c == '{' || c == '}')
					break;
				token.push_back(c);
			}
			_tokens.push_back(token);
		}

		_size = _tokens.size();
	}

	inline bool hasNext() const {
		return _posIndex < _tokens.size();
	}

	inline const std::string& next() {
		core_assert(hasNext());
		return _tokens[_posIndex++];
	}

	inline bool hasPrev() const {
		return _posIndex > 0;
	}

	inline std::size_t size() const {
		return _tokens.size();
	}

	/**
	 * @return the current position in the tokens
	 */
	inline std::size_t pos() const {
		return _posIndex;
	}

	inline const std::string& prev() {
		core_assert(hasPrev());
		return _tokens[--_posIndex];
	}
};

}
