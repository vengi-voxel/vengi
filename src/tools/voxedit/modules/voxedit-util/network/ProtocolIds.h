/**
 * @file
 */
#pragma once

#include "network/ProtocolMessage.h"

namespace voxedit {

// keep alive message
const network::ProtocolId PROTO_PING = 0;
// request the initial scene state from the first client that connects
const network::ProtocolId PROTO_SCENE_STATE_REQUEST = 1;
// the complete scene state - the answer to the scene state request
const network::ProtocolId PROTO_SCENE_STATE = 2;
// voxel modification message with compressed voxel data that is broadcasted to all clients
const network::ProtocolId PROTO_VOXEL_MODIFICATION = 3;
// node added to the scene graph
const network::ProtocolId PROTO_NODE_ADDED = 4;
// node removed from the scene graph
const network::ProtocolId PROTO_NODE_REMOVED = 5;
// node moved in the scene graph to a new parent
const network::ProtocolId PROTO_NODE_MOVED = 6;
// node was renamed
const network::ProtocolId PROTO_NODE_RENAMED = 7;
// node palette was changed
const network::ProtocolId PROTO_NODE_PALETTE_CHANGED = 8;
// node properties were changed
const network::ProtocolId PROTO_NODE_PROPERTIES = 9;
// node key frames for a given animation were changed
const network::ProtocolId PROTO_NODE_KEYFRAMES = 10;
// initial session handshake message
const network::ProtocolId PROTO_INIT_SESSION = 11;
// allow to send commands to the server
const network::ProtocolId PROTO_COMMAND = 12;
// node normal palette was changed
const network::ProtocolId PROTO_NODE_NORMAL_PALETTE_CHANGED = 13;
// scene graph animation list changed
const network::ProtocolId PROTO_SCENE_GRAPH_ANIMATION = 14;
// request the list of available lua scripts
const network::ProtocolId PROTO_LUA_SCRIPTS_REQUEST = 15;
// response with the list of available lua scripts
const network::ProtocolId PROTO_LUA_SCRIPTS_LIST = 16;
// create/send a new lua script
const network::ProtocolId PROTO_LUA_SCRIPT_CREATE = 17;
// node IK constraint was changed
const network::ProtocolId PROTO_NODE_IK_CONSTRAINT = 18;
// log message from the server to the client
const network::ProtocolId PROTO_LOG = 19;

} // namespace voxedit
