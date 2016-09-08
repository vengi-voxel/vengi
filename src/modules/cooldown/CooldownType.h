/**
 * @file
 */

#pragma once

#include <functional>
#include <string>
#include "core/EnumHash.h"
#include "Shared_generated.h"

namespace cooldown {

using Type = network::CooldownType;

inline Type getType(const char* name) {
	const char **names = network::EnumNamesCooldownType();
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
