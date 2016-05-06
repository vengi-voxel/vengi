/**
 * @file
 */

#pragma once

#include <string>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <stdarg.h>
#include <SDL.h>

namespace core {
namespace string {

inline bool isUTF8Multibyte(char c) {
	return (c & 0xc0) == 0x80;
}

inline size_t getUTF8LengthForCharacter(const unsigned char c) {
	if (c < 0x80)
		return 1;
	if (c < 0xc0)
		return 0;
	if (c < 0xe0)
		return 2;
	if (c < 0xf0)
		return 3;
	if (c < 0xf8)
		return 4;
	/* 5 and 6 byte sequences are no longer valid. */
	return 0;
}

inline size_t getUTF8LengthForInt(int c) {
	if (c <= 0x7F)
		return 1;
	if (c <= 0x07FF)
		return 2;
	if (c <= 0xFFFF)
		return 3;
	if (c <= 0x10FFFF) /* highest defined Unicode code */
		return 4;
	return 0;
}

inline int getUTF8Next (const char** str)
{
	const char* s = *str;
	if (s[0] == '\0')
		return -1;

	const unsigned char* buf = reinterpret_cast<const unsigned char*>(s);

	int character;
	int min;
	if (buf[0] < 0x80) {
		min = 0;
		character = buf[0];
	} else if (buf[0] < 0xc0) {
		return -1;
	} else if (buf[0] < 0xe0) {
		min = 1 << 7;
		character = buf[0] & 0x1f;
	} else if (buf[0] < 0xf0) {
		min = 1 << (5 + 6);
		character = buf[0] & 0x0f;
	} else if (buf[0] < 0xf8) {
		min = 1 << (4 + 6 + 6);
		character = buf[0] & 0x07;
	} else {
		return -1;
	}

	const int utf8Length = getUTF8LengthForCharacter(buf[0]);
	for (int i = 1; i < utf8Length; ++i) {
		if (!isUTF8Multibyte(buf[i]))
			return -1;
		character = (character << 6) | (buf[i] & 0x3F);
	}

	if (character < min)
		return -1;

	if (0xD800 <= character && character <= 0xDFFF)
		return -1;

	if (0x110000 <= character)
		return -1;

	*str += utf8Length;
	return character;
}

inline size_t getUTF8Length(const std::string& str) {
	size_t result = 0;
	const char *string = str.c_str();

	while (string[0] != '\0') {
		const int n = getUTF8LengthForCharacter((const unsigned char) *string);
		string += n;
		result++;
	}
	return result;
}

extern std::string format(const char *msg, ...)  SDL_PRINTF_VARARG_FUNC(1);

inline int toInt(const std::string& str) {
	return SDL_atoi(str.c_str());
}

inline int toLong(const std::string& str) {
	return ::atol(str.c_str());
}

inline bool toBool(const std::string& str) {
	return str == "1" || str == "true";
}

inline float toFloat(const std::string& str) {
	return SDL_atof(str.c_str());
}

inline void splitString(const std::string& string, std::vector<std::string>& tokens, const std::string& delimiters = " \t\r\n\f\v") {
	// Skip delimiters at beginning.
	std::string::size_type lastPos = string.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = string.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		tokens.push_back(string.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = string.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = string.find_first_of(delimiters, lastPos);
	}
}

inline std::string toLower(const std::string& string) {
	std::string convert = string;
	std::transform(convert.begin(), convert.end(), convert.begin(), (int (*)(int)) std::tolower);
	return convert;
}

inline bool startsWith(const std::string& string, const std::string& token) {
	return !string.compare(0, token.size(), token);
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

inline std::string replaceAll(const std::string& str, const std::string& searchStr, const std::string& replaceStr) {
	if (str.empty())
		return str;
	std::string sNew = str;
	std::string::size_type loc;
	const std::string::size_type replaceLength = replaceStr.length();
	const std::string::size_type searchLength = searchStr.length();
	std::string::size_type lastPosition = 0;
	while (std::string::npos != (loc = sNew.find(searchStr, lastPosition))) {
		sNew.replace(loc, searchLength, replaceStr);
		lastPosition = loc + replaceLength;
	}
	return sNew;
}

inline std::string cutAfterFirstMatch(const std::string& str, const std::string& pattern, size_t start = 0) {
	std::string::size_type pos = str.find_first_of(pattern, 0);
	return str.substr(start, pos);
}

inline std::string eraseAllSpaces(const std::string& str) {
	std::string tmp = str;
	tmp.erase(std::remove(tmp.begin(), tmp.end(), ' '), tmp.end());
	return tmp;
}

inline bool contains(const std::string& str, const std::string& search) {
	return str.rfind(search) != std::string::npos;
}

inline std::string ltrim(const std::string &str) {
	size_t startpos = str.find_first_not_of(" \t");
	if (std::string::npos != startpos) {
		return str.substr(startpos);
	}
	return str;
}

inline std::string rtrim(const std::string &str) {
	size_t endpos = str.find_last_not_of(" \t");
	if (std::string::npos != endpos) {
		return str.substr(0, endpos + 1);
	}
	return str;
}

inline std::string trim(const std::string &str) {
	return ltrim(rtrim(str));
}

}
}
