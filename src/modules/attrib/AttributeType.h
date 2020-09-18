/**
 * @file
 */

#pragma once

#include "shared/ProtocolEnum.h"
#include "core/String.h"

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
inline Type getType(const core::String& name) {
	return getType(name.c_str());
}

}
