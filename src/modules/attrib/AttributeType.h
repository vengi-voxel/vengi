/**
 * @file
 */

#pragma once

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
