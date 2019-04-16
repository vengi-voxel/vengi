/**
 * @file
 */

#pragma once

#include <string>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <limits.h>
#include <algorithm>
#include <vector>
#include <ctype.h>
#include <stdarg.h>
#include <SDL.h>
#include <string_view>

namespace core {
namespace string {

extern std::string format(SDL_PRINTF_FORMAT_STRING const char *msg, ...) SDL_PRINTF_VARARG_FUNC(1);

inline int toInt(const char*str) {
	return SDL_atoi(str);
}

inline int64_t toLong(const char* str) {
	return ::atol(str);
}

inline int toInt(const std::string& str) {
	return toInt(str.c_str());
}

inline int64_t toLong(const std::string& str) {
	return toLong(str.c_str());
}

inline bool toBool(const std::string& str) {
	return str == "1" || str == "true";
}

inline float toFloat(const std::string& str) {
	return (float)SDL_atof(str.c_str());
}

extern void splitString(const std::string& string, std::vector<std::string>& tokens, const std::string& delimiters = " \t\r\n\f\v");
extern void splitString(const std::string& string, std::vector<std::string_view>& tokens, const std::string& delimiters = " \t\r\n\f\v");

extern std::string toLower(const std::string& string);
extern std::string toLower(const char* string);

extern std::string toUpper(const std::string& string);
extern std::string toUpper(const char* string);

inline bool startsWith(const std::string& string, const std::string& token) {
	return !string.compare(0, token.size(), token);
}

inline bool startsWith(const std::string_view& string, const std::string& token) {
	return !string.compare(0, token.size(), token);
}

inline bool startsWith(const std::string_view& string, const char* token) {
	return !string.compare(0, strlen(token), token);
}

inline bool startsWith(const std::string& string, const char* token) {
	return !string.compare(0, strlen(token), token);
}

inline bool startsWith(const char* string, const char* token) {
	return !std::string_view(string).compare(0, strlen(token), token);
}

/**
 * @brief Locate the string after the last occurrence of the given character in the input string
 * @return nullptr if the character is not part of the input string. Otherwise the pointer to the character
 * followed by the last found match.
 */
inline const char* after(const char* input, int character) {
	const char *s = strrchr(input, character);
	if (s != nullptr) {
		++s;
	}
	return s;
}

inline bool endsWith(const std::string& string, const std::string& end) {
	const std::size_t strLength = string.length();
	const std::size_t endLength = end.length();
	if (strLength >= endLength) {
		const std::size_t index = strLength - endLength;
		return string.compare(index, endLength, end) == 0;
	}
	return false;
}

extern std::string replaceAll(const std::string& str, const std::string& searchStr, const char* replaceStr, size_t replaceStrSize);

inline std::string replaceAll(const std::string& str, const std::string& searchStr, const char* replaceStr) {
	return replaceAll(str, searchStr, replaceStr, strlen(replaceStr));
}

inline std::string replaceAll(const std::string& str, const std::string& searchStr, const std::string_view& replaceStr) {
	return replaceAll(str, searchStr, replaceStr.data(), replaceStr.size());
}

inline std::string replaceAll(const std::string& str, const std::string& searchStr, const std::string& replaceStr) {
	return replaceAll(str, searchStr, replaceStr.data(), replaceStr.size());
}

/**
 * @brief Cuts everything (including the pattern) from the match
 */
inline std::string_view cutAfterFirstMatch(const std::string_view str, const std::string& pattern, size_t start = 0) {
	std::string_view::size_type pos = str.find_first_of(pattern, 0);
	return str.substr(start, pos);
}

/**
 * @brief extract path with trailing /
 * @note Assumed to be normalized (no \ , only /)
 */
inline std::string_view extractPath(const std::string_view str) {
	const size_t pos = str.rfind("/");
	if (pos == std::string::npos) {
		return "";
	}
	return str.substr(0, pos + 1) ;
}

inline std::string_view stripExtension(const std::string_view str) {
	const size_t pos = str.rfind(".");
	if (pos == std::string::npos) {
		return str;
	}
	return str.substr(0, pos) ;
}

inline std::string_view extractFilename(std::string_view str) {
	const size_t pathPos = str.rfind('/');
	if (pathPos != std::string::npos) {
		str = str.substr(pathPos + 1);
	}
	const size_t extPos = str.rfind('.');
	if (extPos != std::string::npos) {
		str = str.substr(0, extPos);
	}
	return str;
}

inline std::string eraseAllSpaces(const std::string& str) {
	std::string tmp = str;
	tmp.erase(std::remove(tmp.begin(), tmp.end(), ' '), tmp.end());
	return tmp;
}

inline bool contains(const std::string& str, const std::string& search) {
	return str.rfind(search) != std::string::npos;
}

inline bool icontains(const std::string& str, const std::string& search) {
	return toLower(str).rfind(toLower(search)) != std::string::npos;
}

inline std::string_view ltrim(const std::string_view str) {
	size_t startpos = str.find_first_not_of(" \t");
	if (std::string::npos != startpos) {
		return str.substr(startpos);
	}
	return str;
}

inline std::string_view rtrim(const std::string_view str) {
	size_t endpos = str.find_last_not_of(" \t");
	if (std::string::npos != endpos) {
		return str.substr(0, endpos + 1);
	}
	return str;
}

inline std::string_view trim(const std::string_view str) {
	return ltrim(rtrim(str));
}

inline bool iequals(const std::string& a, const std::string& b) {
	const unsigned int sz = a.size();
	if (b.size() != sz) {
		return false;
	}
	for (unsigned int i = 0u; i < sz; ++i) {
		if (tolower(a[i]) != tolower(b[i])) {
			return false;
		}
	}
	return true;
}

template<typename ITER>
std::string join(const ITER& begin, const ITER& end, const char *delimiter) {
	std::stringstream ss;
	auto i = begin;
	ss << *i;
	for (++i; i != end; ++i) {
		ss << delimiter;
		ss << *i;
	}
	return ss.str();
}

template<typename ITER, typename FUNC>
std::string join(const ITER& begin, const ITER& end, const char *delimiter, FUNC&& func) {
	std::stringstream ss;
	auto i = begin;
	ss << func(*i);
	for (++i; i != end; ++i) {
		ss << delimiter;
		ss << func(*i);
	}
	return ss.str();
}

/**
 * @brief Performs a pattern/wildcard based string match
 * @param[in] pattern The pattern can deal with wildcards like * and ?
 * @param[in] text The text to match against the pattern
 */
extern bool matches(const char* pattern, const char* text);
extern bool matches(const std::string& pattern, const char* text);
inline bool matches(const std::string& pattern, const std::string& text) {
	return matches(pattern, text.c_str());
}

// pass by copy to prevent aliasing
extern std::string concat(std::string_view first, std::string_view second);

/**
 * @param[in,out] str Converts a string into UpperCamelCase.
 * @note Underscores are removed and the following character is also converted to upper case.
 * Example: @c foo_bar will end as @c FooBar
 */
extern void upperCamelCase(std::string& str);
extern std::string upperCamelCase(const std::string& str);

extern void lowerCamelCase(std::string& str);
extern std::string lowerCamelCase(const std::string& str);

extern char* append(char* buf, size_t bufsize, const char* string);

extern int count(const char *buf, char chr);

}
}
