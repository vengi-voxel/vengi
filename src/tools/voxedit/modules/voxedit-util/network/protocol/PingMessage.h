/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolMessage.h"

namespace voxedit {
namespace network {

/**
 * @brief Protocol keep alive message
 */
PROTO_MSG(PingMessage, PROTO_PING);

} // namespace network
} // namespace voxedit
