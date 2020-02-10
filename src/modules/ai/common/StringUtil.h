/**
 * @file
 */
#pragma once

#include "Math.h"
#include "core/String.h"

namespace ai {
namespace Str {

inline core::String toString(const glm::vec3& pos) {
	char buf[128];
	SDL_snprintf(buf, sizeof(buf), "%f:%f:%f", pos.x, pos.y, pos.z);
	return buf;
}

inline core::String eraseAllSpaces(const core::String& str) {
	if (str.empty()) {
		return str;
	}
	core::String tmp;
	tmp.reserve(str.size() + 1);
	for (auto c : str) {
		if (c == ' ') {
			continue;
		}
		tmp += c;
	}
	return tmp;
}

}
}
