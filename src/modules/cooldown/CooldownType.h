/**
 * @file
 */

#pragma once

#include "network/ProtocolEnum.h"

namespace cooldown {

using Type = network::CooldownType;

inline Type getType(const char* name) {
	return network::getEnum<Type>(name, network::EnumNamesCooldownType());
}

}
