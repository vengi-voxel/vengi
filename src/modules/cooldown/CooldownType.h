/**
 * @file
 */

#pragma once

#include "network/ProtocolEnum.h"
#include <string>

namespace cooldown {

using Type = network::CooldownType;

inline Type getType(const char* name) {
	return network::getEnum<Type>(name, network::EnumNamesCooldownType());
}

inline Type getType(const std::string& name) {
	return getType(name.c_str());
}

}
