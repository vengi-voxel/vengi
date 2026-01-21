/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <cctype>
#include <inttypes.h>

namespace core {
namespace string {

core::String sanitizeFilename(const core::String& input);
core::String humanSize(uint64_t bytes);
inline bool isspace(int c) {
	return ::isspace(c);
}
size_t levenshteinDistance(const core::String &source, const core::String &target);
int toInt(const char* str);

inline int toInt(const core::String& str) {
	return toInt(str.c_str());
}

int64_t toLong(const char* str);
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

core::String toHex(int32_t number);

float toFloat(const core::String& str);

double toDouble(const core::String& str);

double toDouble(const char* str);

/**
 * @brief Modifies the string input buffer by looking for the given token, replace the start of the token with @c \0
 * and returns the position after the given token.
 *
 * @code
 * char *buf = "a b";
 * char *a = getBeforeToken(buf, " ", strlen(buf)); // "a"
 * char *b = buf;                                   // "b"
 * @endcode
 *
 * @return The function returns @c nullptr if the token wasn't found inside the given buffer boundaries.
 */
char* getBeforeToken(char **buffer, const char *token, size_t bufferSize);

/**
 * @note doesn't create empty tokens for leading or trailing delimiters. E.g. ///foo/bar/ having / as delimiter would
 * only return foo and bar - no empty strings - but having /foo//bar/ would return three strings, foo, "" and bar
 */
void splitString(const core::String& string, core::DynamicArray<core::String>& tokens, const char* delimiters = " \t\r\n\f\v");

char toUpper(char in);
char toLower(char in);

inline bool startsWith(const core::String& string, const core::String& token) {
	if (string.size() < token.size()) {
		return false;
	}
	return !string.compare(0, token.size(), token);
}

size_t len(const char *string);

bool startsWith(const core::String& string, const char* token);

bool startsWith(const char* string, const char* token);

/**
 * @brief Locate the string after the last occurrence of the given character in the input string
 * @return nullptr if the character is not part of the input string. Otherwise the pointer to the character
 * followed by the last found match.
 */
const char* after(const char* input, int character);

bool endsWith(const core::String& string, const core::String& end);
bool endsWith(const core::String& string, char end);

core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr, size_t replaceStrSize);
core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr);
core::String replaceAll(const core::String& str, const core::String& searchStr, const core::String& replaceStr);

void replaceAllChars(core::String& str, char in, char out);
void replaceAllChars(char* str, char in, char out);

bool isNumber(const core::String &in);
bool isInteger(const core::String& in);
bool isIntegerWithPostfix(const core::String& in);
bool isAlphaNum(int c);
bool isAlpha(int c);
bool isAbsolutePath(const core::String &in);
bool isRootPath(const core::String &in);
core::String addPostfixToFile(const core::String &filename, const core::String &postfix);

char *strncpyz(const char *input, size_t inputSize, char *target, size_t targetSize);

/**
 * @brief Ensure that exactly one / is at the end of the given path
 * @sa io::normalizePath()
 */
core::String sanitizeDirPath(core::String str);
core::String sanitizePath(const core::String &path);

/**
 * @brief Assembles a string with path separators between the given values
 */
template <typename... ArgTypes>
inline core::String path(ArgTypes... args);

/**
 * @brief Assembles a path from the given components terminated with a trailing /
 */
template <typename T, typename... ArgTypes>
inline core::String path(T t, ArgTypes... args) {
	const core::String &p = path(args...);
	if (p.empty()) {
		return core::String(t);
	}
	core::String first(t);
	if (first.empty()) {
		return p;
	}
	core::String dir = sanitizeDirPath(first);
	if (p.first() == '/') {
		// two were given, don't add the second one
		dir.append(p.substr(1));
	} else {
		dir.append(p);
	}
	return dir;
}

template <>
inline core::String path() {
	return core::String::Empty;
}

bool isSamePath(const core::String &a, const core::String &b);

/**
 * @brief extract path with trailing /
 * @note Assumed to be normalized (no \ , only /)
 */
core::String extractDir(const core::String& str);
// remove all characters that are not alphanumeric and convert everything to lower case
core::String cleanPath(const core::String &str);
core::String stripExtension(const core::String& str);
core::String replaceExtension(const core::String &filename, const core::String& newExtension);
core::String extractExtension(const core::String& str);
core::String extractAllExtensions(const core::String& str);
/**
 * @return the file name of a file path
 * example: given input is @c /foo/bar/file.txt - the result is @c file.txt
 */
core::String extractFilenameWithExtension(const core::String& str);
core::String addFilenamePrefix(const core::String& filename, const core::String &prefix);

bool contains(const char *haystack, const char *needle);

inline bool contains(const core::String& str, const core::String& search) {
	return contains(str.c_str(), search.c_str());
}

/**
 * @return the base name of a file path
 * example: given input is @c /foo/bar/file.txt - the result is @c file
 */
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
	return core::String("No toString implementation", 26);
}

template<>
inline core::String toString(const uint16_t& v) {
	return core::String::format("%u", v);
}

template<>
inline core::String toString(const int16_t& v) {
	return core::String::format("%i", v);
}

template<>
inline core::String toString(const uint32_t& v) {
	return core::String::format("%u", v);
}

template<>
inline core::String toString(const int32_t& v) {
	return core::String::format("%i", v);
}

template<>
inline core::String toString(const float& v) {
	return core::String::format("%f", v);
}

template<>
inline core::String toString(const bool& v) {
	return v ? core::String("true", 4) : core::String("false", 5);
}

template<>
inline core::String toString(const core::String& v) {
	return v;
}

template<>
inline core::String toString(const double& v) {
	return core::String::format("%f", v);
}

template<>
inline core::String toString(const int64_t& v) {
	return core::String::format("%" SDL_PRIs64, v);
}

template<>
inline core::String toString(const uint64_t& v) {
	return core::String::format("%" SDL_PRIu64, v);
}

inline core::String trim(const core::String& str) {
	return str.trim();
}

bool iequals(const core::String& a, const core::String& b);

template<typename ITER>
core::String join(const ITER& begin, const ITER& end, const char *delimiter) {
	auto i = begin;
	if (i == end) {
		return core::String::Empty;
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
		return core::String::Empty;
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
bool matches(const char* text, const char* pattern, bool ignoreCase = true);
bool matches(const char* text, const core::String& pattern, bool ignoreCase = true);
inline bool matches(const core::String& text, const core::String& pattern, bool ignoreCase = true) {
	return matches(text.c_str(), pattern, ignoreCase);
}

/**
 * @param[in] patterns are separated by a comma. Example *.vox,*.qb,*.mcr
 * @note Only for file extensions
 * @return @c true if any of the given patterns matched with the given input text
 */
bool fileMatchesMultiple(const char* text, const char* patterns, bool ignoreCase = true);

/**
 * @param[in,out] str Converts a string into UpperCamelCase.
 * @note Underscores are removed and the following character is also converted to upper case.
 * Example: @c foo_bar will end as @c FooBar
 */
void upperCamelCase(core::String& str);
core::String upperCamelCase(const core::String& str);

void lowerCamelCase(core::String& str);
core::String lowerCamelCase(const core::String& str);

char* append(char* buf, size_t bufsize, const char* string);

int count(const char *buf, char chr);

core::String eraseAllChars(const core::String& str, char chr);
core::String removeAnsiColors(const char* message);
core::String urlEncode(const core::String &inBuf);
/**
 * @sa urlEncode()
 * @brief Replace parts of an url path like /path/to#&/foo - but keeps the slashes
 * in place. Meaning the given example would return /path/to%23%26/foo
 */
core::String urlPathEncode(const core::String &urlPath);

int parseHex(const char *hex, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a);

// taken from tiny_obj_loader - sscanf is locale dependent
void parseReal2(float *x, float *y, const char **token, float default_x = 0.0f,
					   float default_y = 0.0f);
void parseReal3(float *x, float *y, float *z, const char **token, float default_x = 0.0f,
					   float default_y = 0.0f, float default_z = 0.0f);
bool parseReal(const char **token, float *out);
void parseIVec3(const core::String &in, int32_t *out, const char* delimiters = " \t\r\n\f\v");
void parseVec3(const core::String &in, float *out, const char* delimiters = " \t\r\n\f\v");

bool isUrl(const core::String &in);

}
}
