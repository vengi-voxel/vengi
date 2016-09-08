/**
 * @file
 */

#pragma once

#include <cstring>
#include "core/Common.h"
#include "network/ProtocolEnum.h"

namespace attrib {

using Type = network::AttribType;

inline Type getType(const char* name) {
	return network::getEnum<Type>(name, network::EnumNamesAttribType());
}

}
