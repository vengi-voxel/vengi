/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <SDL_stdinc.h>
#include <inttypes.h>

namespace core {
namespace string {

extern core::String format(CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern bool formatBuf(char *buf, size_t bufSize, CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(3);
extern core::String humanSize(uint64_t bytes);

inline int toInt(const char* str) {
	return SDL_atoi(str);
}

inline int toInt(const core::String& str) {
	return toInt(str.c_str());
}

extern int64_t toLong(const char* str);
inline int64_t toLong(const core::String& str) {
	return toLong(str.c_str());
}

inline bool toBool(const core::String& str) {
	return str == "1" || str == "true";
}

inline char toHex(char code) {
	static const char* _hex = "0123456789ABCDEF";
	return _hex[code & 15];
}

inline float toFloat(const core::String& str) {
	return (float)SDL_atof(str.c_str());
}

inline double toDouble(const core::String& str) {
	return (double)SDL_atof(str.c_str());
}

inline double toDouble(const char* str) {
	return (double)SDL_atof(str);
}

/**
 * @brief Modifies the string input buffer by looking for the given token, replace the start of the token with @c \0
 * and returns the position after the given token.
 *
 * @code
 * char *buf = "a b";
 * char *a = getBeforeToken(buf, " ", SDL_strlen(buf)); // "a"
 * char *b = buf;                                   // "b"
 * @endcode
 *
 * @return The function returns @c nullptr if the token wasn't found inside the given buffer boundaries.
 */
extern char* getBeforeToken(char **buffer, const char *token, size_t bufferSize);

extern void splitString(const core::String& string, core::DynamicArray<core::String>& tokens, const char* delimiters = " \t\r\n\f\v");

inline char toUpper(char in) { return (char)SDL_toupper((int)in); }
inline char toLower(char in) { return (char)SDL_tolower((int)in); }

inline bool startsWith(const core::String& string, const core::String& token) {
	if (string.size() < token.size()) {
		return false;
	}
	return !string.compare(0, token.size(), token);
}

inline bool startsWith(const core::String& string, const char* token) {
	const size_t tokensize = SDL_strlen(token);
	if (string.size() < tokensize) {
		return false;
	}
	return !string.compare(0, tokensize, token);
}

inline bool startsWith(const char* string, const char* token) {
	return !SDL_strncmp(string, token, SDL_strlen(token));
}

/**
 * @brief Locate the string after the last occurrence of the given character in the input string
 * @return nullptr if the character is not part of the input string. Otherwise the pointer to the character
 * followed by the last found match.
 */
inline const char* after(const char* input, int character) {
	const char *s = SDL_strrchr(input, character);
	if (s != nullptr) {
		++s;
	}
	return s;
}

inline bool endsWith(const core::String& string, const core::String& end) {
	const size_t strLength = string.size();
	const size_t endLength = end.size();
	if (strLength >= endLength) {
		const size_t index = strLength - endLength;
		return string.compare(index, endLength, end) == 0;
	}
	return false;
}

extern core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr, size_t replaceStrSize);

inline core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr) {
	return replaceAll(str, searchStr, replaceStr, SDL_strlen(replaceStr));
}

extern void replaceAllChars(core::String& str, char in, char out);

inline core::String replaceAll(const core::String& str, const core::String& searchStr, const core::String& replaceStr) {
	if (searchStr.size() == 1 && replaceStr.size() == 1) {
		core::String copy = str;
		replaceAllChars(copy, searchStr[0], replaceStr[0]);
		return copy;
	}
	return replaceAll(str, searchStr, replaceStr.c_str(), replaceStr.size());
}

extern bool isNumber(const core::String &in);
extern bool isInteger(const core::String& in);

/**
 * @brief extract path with trailing /
 * @note Assumed to be normalized (no \ , only /)
 */
inline core::String extractPath(const core::String& str) {
	const size_t pos = str.rfind("/");
	if (pos == core::String::npos) {
		return "";
	}
	return str.substr(0, pos + 1) ;
}

inline core::String stripExtension(const core::String& str) {
	const size_t pos = str.rfind(".");
	if (pos == core::String::npos) {
		return str;
	}
	return str.substr(0, pos) ;
}

inline core::String extractExtension(const core::String& str) {
	const size_t pos = str.rfind(".");
	if (pos == core::String::npos) {
		return "";
	}
	return str.substr(pos + 1) ;
}

inline core::String extractFilenameWithExtension(const core::String& str) {
	const size_t pathPos = str.rfind('/');
	if (pathPos == core::String::npos) {
		return str;
	}
	return str.substr(pathPos + 1);
}

inline bool contains(const char *haystack, const char *needle) {
	const char *pos = (const char*)SDL_strstr(haystack, needle);
	return pos != nullptr;
}

inline bool contains(const core::String& str, const core::String& search) {
	return contains(str.c_str(), search.c_str());
}

inline core::String extractFilename(const core::String& str) {
	return stripExtension(extractFilenameWithExtension(str));
}

inline bool icontains(const core::String& str, const core::String& search) {
	const core::String& lower = search.toLower();
	const core::String& lowerStr = str.toLower();
	return lowerStr.rfind(lower.c_str()) != core::String::npos;
}

template<class T>
inline core::String toString(const T& v) {
	return core::String("No toString implementation");
}

template<>
inline core::String toString(const unsigned short& v) {
	return core::String::format("%u", v);
}

template<>
inline core::String toString(const short& v) {
	return core::String::format("%i", v);
}

template<>
inline core::String toString(const unsigned int& v) {
	return core::String::format("%u", v);
}

template<>
inline core::String toString(const int& v) {
	return core::String::format("%i", v);
}

template<>
inline core::String toString(const float& v) {
	return core::String::format("%f", v);
}

template<>
inline core::String toString(const double& v) {
	return core::String::format("%f", v);
}

template<>
inline core::String toString(const int64_t& v) {
#ifdef _MSC_VER
	return core::String::format("%lld", (long long)v);
#else
	return core::String::format("%" PRId64, v);
#endif
}

// win32 doesn't like to combine this with the unsigned int override
#if defined(_WIN64) || !defined(_WIN32)
template <> inline core::String toString(const size_t &v) {
#ifdef _MSC_VER
#if defined(_WIN64)
	return core::String::format("%lld", (long long)v);
#else
	return core::String::format("%u", v);
#endif
#else
	return core::String::format("%" PRIu64, (uint64_t)v);
#endif
}
#endif

inline core::String trim(const core::String& str) {
	return str.trim();
}

inline bool iequals(const core::String& a, const core::String& b) {
	const size_t sz = a.size();
	if (b.size() != sz) {
		return false;
	}
	for (size_t i = 0u; i < sz; ++i) {
		if (SDL_tolower(a[i]) != SDL_tolower(b[i])) {
			return false;
		}
	}
	return true;
}

template<typename ITER>
core::String join(const ITER& begin, const ITER& end, const char *delimiter) {
	auto i = begin;
	if (i == end) {
		return "";
	}
	core::String ss;
	ss += *i;
	for (++i; i != end; ++i) {
		ss += delimiter;
		ss += *i;
	}
	return ss;
}

template<typename ITER, typename FUNC>
core::String join(const ITER& begin, const ITER& end, const char *delimiter, FUNC&& func) {
	auto i = begin;
	if (i == end) {
		return "";
	}
	core::String ss;
	ss += func(*i);
	for (++i; i != end; ++i) {
		ss += delimiter;
		ss += func(*i);
	}
	return ss;
}

/**
 * @brief Performs a pattern/wildcard based string match
 * @param[in] pattern The pattern can deal with wildcards like * and ?
 * @param[in] text The text to match against the pattern
 */
extern bool matches(const char* pattern, const char* text);
extern bool matches(const core::String& pattern, const char* text);
inline bool matches(const core::String& pattern, const core::String& text) {
	return matches(pattern, text.c_str());
}

/**
 * @note patterns are separated by a comma. Example *.vox,*.qb,*.mcr
 * @return @c true if any of the given patterns matched with the given input text
 */
extern bool fileMatchesMultiple(const char* patterns, const char* text);

/**
 * @param[in,out] str Converts a string into UpperCamelCase.
 * @note Underscores are removed and the following character is also converted to upper case.
 * Example: @c foo_bar will end as @c FooBar
 */
extern void upperCamelCase(core::String& str);
extern core::String upperCamelCase(const core::String& str);

extern void lowerCamelCase(core::String& str);
extern core::String lowerCamelCase(const core::String& str);

extern char* append(char* buf, size_t bufsize, const char* string);

extern int count(const char *buf, char chr);

extern core::String eraseAllChars(const core::String& str, char chr);

/**
 * @note Call @c core_free() on the returned string
 */
extern char *urlEncode(const char *str);

}
}
