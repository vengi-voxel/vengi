/**
 * @file
 */

#pragma once

#include "network/ProtocolEnum.h"
#include <string>

namespace attrib {

using Type = network::AttribType;

inline Type getType(const char* name) {
	return network::getEnum<Type>(name, network::EnumNamesAttribType());
}

inline Type getType(const std::string& name) {
	return getType(name.c_str());
}

}
