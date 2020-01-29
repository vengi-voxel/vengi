/**
 * @file
 */
#pragma once

#include "Math.h"
#include <vector>
#include "core/String.h"
#include <sstream>
#include <algorithm>

namespace ai {
namespace Str {

inline core::String toString(const glm::vec3& pos) {
	char buf[128];
	std::snprintf(buf, sizeof(buf), "%f:%f:%f", pos.x, pos.y, pos.z);
	return buf;
}

inline bool startsWith(const core::String& string, const core::String& token) {
	return !string.compare(0, token.size(), token);
}

inline float strToFloat(const core::String& str) {
	return static_cast<float>(::atof(str.c_str()));
}

inline core::String eraseAllSpaces(const core::String& str) {
	if (str.empty())
		return str;
	core::String tmp = str;
	tmp.erase(std::remove(tmp.begin(), tmp.end(), ' '), tmp.end());
	return tmp;
}

inline void splitString(const core::String& string, std::vector<std::string>& tokens, const core::String& delimiters = "()") {
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

}

}
