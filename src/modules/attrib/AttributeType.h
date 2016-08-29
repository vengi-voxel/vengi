/**
 * @file
 */

#pragma once

#include <functional>
#include <cstring>
#include "core/Common.h"
#include "Shared_generated.h"

namespace attrib {

using Type = network::AttribType;

inline Type getType(const char* name) {
	const char **names = network::EnumNamesAttribType();
	int i = 0;
	while (*names) {
		if (!strcmp(*names, name)) {
			return static_cast<Type>(i);
		}
		++i;
		++names;
	}
	return Type::NONE;
}

}

namespace std {
template<> struct hash<attrib::Type> {
	inline size_t operator()(const attrib::Type &s) const {
		return static_cast<size_t>(s);
	}
};
}
