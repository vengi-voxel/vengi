/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/ArrayLength.h"
#include <stddef.h>
#include "core/Common.h"


namespace core {

class String {
private:
	struct data {
		// the string length cached
		size_t _size = 0u;
		// pointer to the internal buffer or to the allocated buffer - see @c _capacity
		char *_str = nullptr;
		// capacity of any allocated string buffer
		size_t _capacity = 0u;
	};

	// buffer to prevent memory allocation. If string is longer than the available buffer size,
	// perform dynamic memory allocation
	// ensure proper alignment
	char _buf[64 - sizeof(data)] = "";
	data _data {0u, _buf, 0u};
	static const constexpr size_t _stackBufCapacity = sizeof(_buf);

	bool onStack() const;
	void copyBuf(const char *buf, size_t len);
	void checkBufferSize(size_t len);
public:
	static constexpr const size_t npos = ~0u;
	/* constexpr */String() {}
	String(size_t len, char chr);
	String(const char *str);
	String(const char *str, size_t len);
	String(const String &str);
	String(String &&str) noexcept;
	~String();

	void reserve(size_t bytes);
	void replaceAllChars(char in, char out);
	String trim() const;
	String substr(size_t index, size_t len = npos) const;
	void replace(size_t index, size_t length, const char *str);

	static String lower(const char *string);
	static String upper(const char *string);

	String toLower() const;
	String toUpper() const;

	const char *c_str() const;
	char *c_str();
	size_t size() const;
	int compare(const String& str) const;
	int compare(const char *str, size_t len) const;
	int compare(const char *str) const;
	int compare(size_t index, size_t len, const String& str) const;
	bool equals(const char *str) const;
	bool empty() const;
	void clear();
	void insert(size_t index, const char *str);
	void insert(size_t index, const char *str, size_t len);

	int toInt() const;
	float toFloat() const;

	bool contains(const String& str) const;
	bool contains(const char *str, size_t len) const;

	const char& operator[](size_t idx) const;
	char& operator[](size_t idx);

	char last() const;

	const char *begin() const;
	const char *end() const;

	size_t rfind(const char *s) const;
	size_t rfind(char c) const;

	/**
	 * Index to the first element that is equal to a character from the given input chars. If no such char is found, @c npos is returned.
	 */
	size_t find_first_of(const char *chars, size_t pos = 0u) const;
	/**
	 * Index to the first element that is equal to the input character. If no such char is found, @c npos is returned.
	 */
	size_t find_first_of(char c, size_t pos = 0u) const;
	size_t find_first_not_of(const char *chars, size_t pos = 0u) const;
	size_t find_last_of(const char *chars, size_t index = 0u) const;
	size_t find(const String &str, size_t pos = 0u) const;
	size_t find(const char *str, size_t pos = 0u) const;

	void erase(size_t index, size_t length = 1u);

	static String format(CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(1);

	String& append(int);
	String& append(float);
	String& append(const char *str);
	String& append(const char *str, size_t len);
	String& append(const String &str);

	String &operator=(const char *str);
	String &operator=(const String &str);
	String &operator=(String &&str) noexcept;
	String &operator=(char c);
	String &operator+=(const char *str);
	String &operator+=(const String &str);
	String &operator+=(char c);
	bool operator==(const String &x) const;
	bool operator==(const char *x) const;
	bool operator!=(const String &x) const;
	bool operator!=(const char *x) const;

	bool operator<(const String &x) const;
	bool operator<=(const String &x) const;
	bool operator>(const String &x) const;
	bool operator>=(const String &x) const;
};

inline bool String::onStack() const {
	return _data._str == _buf;
}

inline const char *String::c_str() const {
	return _data._str;
}

inline char *String::c_str() {
	return _data._str;
}

inline size_t String::size() const {
	return _data._size;
}

inline bool String::empty() const {
	return _data._size == 0;
}

inline const char& String::operator[](size_t idx) const {
	core_assert(_data._str && idx <= _data._size);
	return _data._str[idx];
}

inline char& String::operator[](size_t idx) {
	core_assert(_data._str && idx <= _data._size);
	return _data._str[idx];
}

inline const char *String::begin() const {
	return _data._str;
}

inline const char *String::end() const {
	return begin() + size();
}

String operator+(const String &x, const String &y);
String operator+(const char *x, const String &y);
String operator+(const String &x, const char *y);

bool operator==(const char *x, const String &y);
bool operator!=(const char *x, const String &y);

struct StringHash {
	size_t operator()(const core::String &p) const;
};

}
