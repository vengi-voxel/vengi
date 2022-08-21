/**
 * @file
 */

#pragma once

#include "core/StandardLib.h"
#include "core/String.h"
#include "core/Assert.h"

template<size_t SIZE>
class FixedString {
private:
	char _buf[SIZE];
	static_assert(SIZE >= 2, "SIZE must be >= 2");

	inline void copyBuf(const char *buf) {
		SDL_strlcpy(_buf, buf, SIZE);
	}

public:
	FixedString(const core::String &str) {
		copyBuf(str.c_str());
	}

	FixedString(const char *str) {
		copyBuf(str);
	}

	const char *c_str() const {
		return _buf;
	}

	char *c_str() {
		return _buf;
	}

	operator const char *() const {
		return _buf;
	}

	const char &operator[](size_t idx) const {
		core_assert_msg(idx < SIZE, "Index is out of bounds: %i", (int)idx);
		return _buf[idx];
	}

	char &operator[](size_t idx) {
		core_assert_msg(idx < SIZE, "Index is out of bounds: %i", (int)idx);
		return _buf[idx];
	}

	FixedString &operator=(const char *str) {
		copyBuf(str);
		return *this;
	}

	FixedString &operator=(const FixedString &str) {
		if (&str == this) {
			return *this;
		}
		copyBuf(str.c_str());
		return *this;
	}

	FixedString &operator=(char c) {
		_buf[0] = c;
		_buf[1] = '\0';
		return *this;
	}
};
