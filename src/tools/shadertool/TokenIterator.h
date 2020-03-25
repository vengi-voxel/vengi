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
	int _line = -1;
	int _column = -1;
	const char *_file = nullptr;
	char _op = '\0';
public:
	void init(const simplecpp::TokenList* tokenList) {
		_tokenList = tokenList;
		_tok = _tokenList->cfront();
	}

	inline bool hasNext() const {
		return _tok != nullptr;
	}

	inline core::String next() {
		const core::String token = _tok->str().c_str();
		_line = _tok->location.line;
		_column = _tok->location.col;
		_file = _tok->location.file().c_str();
		_op = _tok->op;
		_tok = _tok->next;
		return token;
	}

	inline bool hasPrev() {
		return _tok != _tokenList->cfront();
	}

	inline core::String prev() {
		if (!_tok) {
			_tok = _tokenList->cback();
		}
		_tok = _tok->previous;
		const core::String token = _tok->str().c_str();
		_line = _tok->location.line;
		_column = _tok->location.col;
		_op = _tok->op;
		_file = _tok->location.file().c_str();
		return token;
	}

	inline char op() const {
		return _op;
	}

	inline int line() const {
		return _line;
	}

	inline const char* file() const {
		return _file;
	}

	inline int col() const {
		return _column;
	}

	inline core::String peekNext() const {
		if (!_tok) {
			return "";
		}
		return _tok->str().c_str();
	}
};

}
