/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Acknowledgment message sent by the server after processing a command
 */
PROTO_MSG(AckMessage, PROTO_ACK);

} // namespace voxedit
