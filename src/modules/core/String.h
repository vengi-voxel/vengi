/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "core/Common.h"

namespace core {

class String {
private:
	struct data {
		// the string length cached
		uint32_t _size = 0u;
		// capacity of any allocated string buffer
		uint32_t _capacity = 0u;
		// pointer to the internal buffer or to the allocated buffer - see @c _capacity
		char *_str = nullptr;
	};

	// buffer to prevent memory allocation. If string is longer than the available buffer size,
	// perform dynamic memory allocation
	// ensure proper alignment
	char _buf[64 - sizeof(data)] = "";
	data _data {0u, 0u, _buf};
	static const constexpr size_t _stackBufCapacity = sizeof(_buf);

	bool onStack() const;
	void copyBuf(const char *buf, size_t len);
	void checkBufferSize(size_t len);
public:
	static const core::String Empty;
	static const size_t npos;
	/* constexpr */String() {}
	String(size_t len, char chr);
	String(const char *str);
	String(const char *str, size_t len);
	String(const String &str);
	String(String &&str) noexcept;
	~String();

	void resize(size_t bytes, char c = '\0');
	void reserve(size_t bytes);
	void replaceAllChars(char in, char out);
	String trim() const;
	String substr(size_t index, size_t len = npos) const;
	void replace(size_t index, size_t length, const char *str);

	static String lower(const char *string);
	static String upper(const char *string);

	String toLower() const;
	String toUpper() const;

	void updateSize();

	const char *c_str() const;
	char *c_str();
	size_t capacity() const;
	size_t size() const;
	/**
	 * 0, if the this string and str are equal;
	 * a negative value if this string is less than str;
	 * a positive value if this string  is greater than s2.
	 */
	int compare(const String& str) const;
	/**
	 * 0, if the this string and str are equal;
	 * a negative value if this string is less than str;
	 * a positive value if this string  is greater than s2.
	 */
	int compare(const char *str, size_t len) const;
	/**
	 * 0, if the this string and str are equal;
	 * a negative value if this string is less than str;
	 * a positive value if this string  is greater than s2.
	 */
	int compare(const char *str) const;
	int compare(size_t index, size_t len, const String& str) const;
	bool equals(const char *str) const;
	bool empty() const;
	void clear();
	// reset size and string, but keep the allocated buffer
	void reset();
	void insert(size_t index, const char *str);
	void insert(size_t index, const char *str, size_t len);

	int toInt() const;
	float toFloat() const;

	bool contains(const String& str) const;
	bool contains(const char *str, size_t len) const;

	const char& operator[](size_t idx) const;
	char& operator[](size_t idx);

	bool pop();
	char last() const;
	char first() const;

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
	static int formatBuf(char *buf, size_t bufSize, CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(3);

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

inline String::String(const String &str) : String(str.c_str(), str.size()) {
}

inline bool String::onStack() const {
	return _data._str == _buf;
}

inline const char *String::c_str() const {
	return _data._str;
}

inline char *String::c_str() {
	return _data._str;
}

inline size_t String::capacity() const {
	if (onStack()) {
		return _stackBufCapacity;
	}
	return _data._capacity;
}

inline size_t String::size() const {
	return _data._size;
}

inline bool String::empty() const {
	return _data._size == 0;
}

inline const char *String::begin() const {
	return _data._str;
}

inline const char *String::end() const {
	return begin() + size();
}

inline size_t String::find(const String &str, size_t pos) const {
	return find(str.c_str(), pos);
}

inline void String::reserve(size_t bytes) {
	checkBufferSize(bytes + 1);
}

inline bool String::contains(const core::String &str) const {
	return contains(str.c_str(), str.size());
}

inline bool String::operator==(const String &rhs) const {
	return equals(rhs.c_str());
}

inline bool String::operator==(const char *rhs) const {
	return equals(rhs);
}

inline bool String::operator!=(const String &rhs) const {
	return !equals(rhs.c_str());
}

inline bool String::operator!=(const char *rhs) const {
	return !equals(rhs);
}

inline bool String::operator<(const String &rhs) const {
	return compare(rhs) < 0;
}

inline bool String::operator<=(const String &rhs) const {
	return compare(rhs) <= 0;
}

inline bool String::operator>(const String &rhs) const {
	return compare(rhs) > 0;
}

inline bool String::operator>=(const String &rhs) const {
	return compare(rhs) >= 0;
}

inline String &String::operator+=(const String &str) {
	return operator+=(str.c_str());
}

inline bool operator==(const char *lhs, const String &rhs) {
	return rhs == lhs;
}

inline bool operator!=(const char *lhs, const String &rhs) {
	return rhs != lhs;
}

String operator+(const String &x, const String &y);
String operator+(const char *x, const String &y);
String operator+(const String &x, const char *y);
String operator+(const String &x, char y);

bool operator==(const char *x, const String &y);
bool operator!=(const char *x, const String &y);

struct StringHash {
	size_t operator()(const core::String &p) const;
};

}
