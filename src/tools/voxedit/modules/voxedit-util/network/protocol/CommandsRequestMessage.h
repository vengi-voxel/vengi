/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Request the list of commands from the server
 */
PROTO_MSG(CommandsRequestMessage, PROTO_COMMANDS_REQUEST);

} // namespace voxedit
