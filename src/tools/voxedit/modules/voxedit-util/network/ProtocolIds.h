/**
 * @file
 */
#pragma once

#include "ProtocolMessage.h"

namespace voxedit {

// keep alive message
const ProtocolId PROTO_PING = 0;
// request the initial scene state from the first client that connects
const ProtocolId PROTO_SCENE_STATE_REQUEST = 1;
// the complete scene state - the answer to the scene state request
const ProtocolId PROTO_SCENE_STATE = 2;
// voxel modification message with compressed voxel data that is broadcasted to all clients
const ProtocolId PROTO_VOXEL_MODIFICATION = 3;
// node added to the scene graph
const ProtocolId PROTO_NODE_ADDED = 4;
// node removed from the scene graph
const ProtocolId PROTO_NODE_REMOVED = 5;
// node moved in the scene graph to a new parent
const ProtocolId PROTO_NODE_MOVED = 6;
// node was renamed
const ProtocolId PROTO_NODE_RENAMED = 7;
// node palette was changed
const ProtocolId PROTO_NODE_PALETTE_CHANGED = 8;
// node properties were changed
const ProtocolId PROTO_NODE_PROPERTIES = 9;
// node key frames for a given animation were changed
const ProtocolId PROTO_NODE_KEYFRAMES = 10;
// initial session handshake message
const ProtocolId PROTO_INIT_SESSION = 11;
// allow to send commands to the server
const ProtocolId PROTO_COMMAND = 12;

} // namespace voxedit
