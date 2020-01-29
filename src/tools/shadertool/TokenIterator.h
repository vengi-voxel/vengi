/**
 * @file
 */

#pragma once

#include <simplecpp.h>

namespace shadertool {

class TokenIterator {
private:
	const simplecpp::TokenList* _tokenList = nullptr;
	const simplecpp::Token *_tok = nullptr;
public:
	void init(const simplecpp::TokenList* tokenList) {
		_tokenList = tokenList;
		_tok = _tokenList->cfront();
	}

	inline bool hasNext() const {
		return _tok != nullptr;
	}

	inline core::String next() {
		const core::String& token = _tok->str();
		_tok = _tok->next;
		return token;
	}

	inline core::String prev() {
		_tok = _tok->previous;
		return _tok->str();
	}

	inline int line() const {
		if (!_tok) {
			return -1;
		}
		return _tok->location.line;
	}

	inline core::String peekNext() const {
		if (!_tok) {
			return "";
		}
		return _tok->str();
	}
};

}
