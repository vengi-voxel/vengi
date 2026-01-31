/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Request the list of cvars from the server
 */
PROTO_MSG(CVarsRequestMessage, PROTO_CVARS_REQUEST);

} // namespace voxedit
