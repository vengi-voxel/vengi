/**
 * @file
 */

#pragma once

#include "network/ProtocolEnum.h"
#include <string>

namespace attrib {

/**
 * @brief The type of each attribute is directly mapped to a protocol enum here.
 * @ingroup Attributes
 */
using Type = ::network::AttribType;

/**
 * @brief Converts a string into the enum value
 * @ingroup Attributes
 */
inline Type getType(const char* name) {
	return ::network::getEnum<Type>(name, ::network::EnumNamesAttribType());
}

/**
 * @brief Converts a string into the enum value
 * @ingroup Attributes
 */
inline Type getType(const std::string& name) {
	return getType(name.c_str());
}

}
