/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {
namespace network {

/**
 * @brief Scene state request message - sent by server to request initial state from client
 */
PROTO_MSG(SceneStateRequestMessage, PROTO_SCENE_STATE_REQUEST);

} // namespace network
} // namespace voxedit
