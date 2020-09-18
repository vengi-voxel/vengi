/**
 * @file
 */

#pragma once

#include "shared/ProtocolEnum.h"
#include "core/String.h"

namespace cooldown {

/**
 * @brief The type of each cooldown is directly mapped to a protocol enum here.
 * @ingroup Cooldowns
 */
using Type = network::CooldownType;

/**
 * @brief Converts a string into the enum value
 * @ingroup Cooldowns
 */
inline Type getType(const char* name) {
	return network::getEnum<Type>(name, network::EnumNamesCooldownType());
}

/**
 * @brief Converts a string into the enum value
 * @ingroup Cooldowns
 */
inline Type getType(const core::String& name) {
	return getType(name.c_str());
}

/**
 * @brief Converts enum value into the string representation
 * @ingroup Cooldowns
 */
inline const char* getType(Type type) {
	return network::EnumNameCooldownType(type);
}

}
