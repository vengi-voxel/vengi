/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Assert.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"

namespace core {

struct TokenizerConfig {
	bool skipComments = true;
	bool removeQuotes = true;
};

class Tokenizer {
protected:
	core::DynamicArray<core::String> _tokens;
	size_t _posIndex;
	size_t _size;
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
	Tokenizer(const TokenizerConfig &cfg, const char* s, size_t len, const char *sep = " (){};", const char *split = "");
	Tokenizer(const char* s, size_t len, const char *sep = " (){};", const char *split = "") : Tokenizer(TokenizerConfig(), s, len, sep, split) {}

	Tokenizer(const TokenizerConfig &cfg, const char* string, const char *sep = " (){};", const char *split = "") : Tokenizer(cfg, string, core::string::len(string), sep, split) {}
	Tokenizer(const char* string, const char *sep = " (){};", const char *split = "") : Tokenizer(TokenizerConfig(), string, core::string::len(string), sep, split) {}
	Tokenizer(const core::String& string, const char *sep = " (){};", const char *split = "") : Tokenizer(TokenizerConfig(), string.c_str(), string.size(), sep, split) {}
	Tokenizer(const TokenizerConfig &cfg, const core::String& string, const char *sep = " (){};", const char *split = "") : Tokenizer(cfg, string.c_str(), string.size(), sep, split) {}

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

	inline const core::DynamicArray<core::String>& tokens() const {
		return _tokens;
	}

	inline bool hasPrev() const {
		return _posIndex > 0;
	}

	inline size_t size() const {
		return _tokens.size();
	}

	/**
	 * @return the current position in the tokens
	 */
	inline size_t pos() const {
		return _posIndex;
	}

	inline const core::String& prev() {
		core_assert(hasPrev());
		return _tokens[--_posIndex];
	}
};

}
