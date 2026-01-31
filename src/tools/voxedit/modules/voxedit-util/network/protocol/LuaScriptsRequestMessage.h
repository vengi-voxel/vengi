/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Request the list of available lua scripts from the server
 */
PROTO_MSG(LuaScriptsRequestMessage, PROTO_LUA_SCRIPTS_REQUEST);

} // namespace voxedit
