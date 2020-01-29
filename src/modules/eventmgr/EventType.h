/**
 * @file
 */

#pragma once

#include "network/ProtocolEnum.h"
#include "core/String.h"

namespace eventmgr {

/**
 * @brief The type of each event is directly mapped to a protocol enum here.
 * @ingroup Events
 */
using Type = network::EventType;

/**
 * @brief Converts a string into the enum value
 * @ingroup Events
 */
inline Type getType(const char* name) {
	return network::getEnum<Type>(name, network::EnumNamesEventType());
}

/**
 * @brief Converts a string into the enum value
 * @ingroup Events
 */
inline Type getType(const std::string& name) {
	return getType(name.c_str());
}

}
