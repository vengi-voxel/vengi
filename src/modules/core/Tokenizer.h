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
	int32_t _len;

	// skip multiline and singleline comments
	bool skipComments(const char **s, bool skipWhitespac);
	char skip(const char **s, bool skipWhitespace = true);
	bool isSeparator(char c, const char *sep);
public:
	Tokenizer(const char* s, std::size_t len, const char *sep = " (){};", const char *split = "");

	Tokenizer(const std::string_view string, const char *sep, const char *split = "") : Tokenizer(string.data(), string.length(), sep, split) {}
	Tokenizer(const char* string, const char *sep = " (){};", const char *split = "") : Tokenizer(string, strlen(string), sep, split) {}
	Tokenizer(const std::string& string, const char *sep = " (){};", const char *split = "") : Tokenizer(string.c_str(), string.size(), sep, split) {}

	inline bool hasNext() const {
		return _posIndex < _tokens.size();
	}

	inline const std::string& next() {
		core_assert(hasNext());
		return _tokens[_posIndex++];
	}

	inline const std::vector<std::string>& tokens() const {
		return _tokens;
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
