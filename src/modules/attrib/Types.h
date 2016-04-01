#pragma once

#include <functional>
#include <cstring>
#include "core/Common.h"

namespace attrib {

enum class Types {
	NONE,
	HEALTH,
	SPEED,
	VIEWDISTANCE,
	ATTACKRANGE,
	STRENGTH,
	MAX
};

static const char* typeNames[] = {
	"NONE",
	"HEALTH",
	"SPEED",
	"VIEWDISTANCE",
	"ATTACKRANGE",
	"STRENGTH"
};
static_assert(SDL_arraysize(typeNames) == (size_t)Types::MAX, "types and names don't match");

inline Types getType(const char* name) {
	for (int i = 0; i < (int)Types::MAX; ++i) {
		if (!strcmp(typeNames[i], name))
			return (Types)i;
	}
	return Types::NONE;
}

}

namespace std {
template<> struct hash<attrib::Types> {
	inline size_t operator()(const attrib::Types &s) const {
		return static_cast<size_t>(s);
	}
};
}
