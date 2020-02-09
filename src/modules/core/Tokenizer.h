/**
 * @file
 */

#pragma once

#include "Common.h"
#include "Assert.h"
#include "StringUtil.h"

namespace core {

class Tokenizer {
protected:
	std::vector<core::String> _tokens;
	std::size_t _posIndex;
	std::size_t _size;
	int32_t _len;
	bool _skipComments;

	// skip multiline and singleline comments
	bool skipComments(const char **s, bool skipWhitespac);
	char skip(const char **s, bool skipWhitespace = true);
	bool isSeparator(char c, const char *sep);
public:
	/**
	 * @param s The string to tokenize.
	 * @param len The length of the string.
	 * @param sep The separator chars - they are not included in the tokens.
	 * @param split Splits chars, they are included in the tokens - but otherwise handled like usual separators.
	 */
	Tokenizer(bool skipComments, const char* s, std::size_t len, const char *sep = " (){};", const char *split = "");
	Tokenizer(const char* s, std::size_t len, const char *sep = " (){};", const char *split = "") : Tokenizer(true, s, len, sep, split) {}

	Tokenizer(bool skipComments, const char* string, const char *sep = " (){};", const char *split = "") : Tokenizer(skipComments, string, SDL_strlen(string), sep, split) {}
	Tokenizer(const char* string, const char *sep = " (){};", const char *split = "") : Tokenizer(true, string, SDL_strlen(string), sep, split) {}
	Tokenizer(const core::String& string, const char *sep = " (){};", const char *split = "") : Tokenizer(true, string.c_str(), string.size(), sep, split) {}
	Tokenizer(bool skipComments, const core::String& string, const char *sep = " (){};", const char *split = "") : Tokenizer(skipComments, string.c_str(), string.size(), sep, split) {}

	inline bool hasNext() const {
		return _posIndex < _tokens.size();
	}

	inline core::String peekNext() const {
		if (!hasNext()) {
			return "";
		}
		return _tokens[_posIndex + 1];
	}

	inline bool isNext(const core::String& token) const {
		if (!hasNext()) {
			return false;
		}
		return _tokens[_posIndex + 1] == token;
	}

	inline const core::String& next() {
		core_assert(hasNext());
		return _tokens[_posIndex++];
	}

	inline const std::vector<core::String>& tokens() const {
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

	inline const core::String& prev() {
		core_assert(hasPrev());
		return _tokens[--_posIndex];
	}
};

}
