/**
 * @file
 */

#include "ProtocolMessageFactory.h"
#include "protocol/InitSessionMessage.h"
#include "protocol/LuaScriptCreateMessage.h"
#include "protocol/LuaScriptsListMessage.h"
#include "protocol/LuaScriptsRequestMessage.h"
#include "protocol/NodeAddedMessage.h"
#include "protocol/NodeKeyFramesMessage.h"
#include "protocol/NodeMovedMessage.h"
#include "protocol/NodePaletteChangedMessage.h"
#include "protocol/NodePropertiesMessage.h"
#include "protocol/NodeRemovedMessage.h"
#include "protocol/NodeRenamedMessage.h"
#include "protocol/NodeNormalPaletteChangedMessage.h"
#include "protocol/SceneGraphAnimationMessage.h"
#include "protocol/PingMessage.h"
#include "protocol/SceneStateMessage.h"
#include "protocol/SceneStateRequestMessage.h"
#include "protocol/VoxelModificationMessage.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxedit-util/network/protocol/CommandMessage.h"

namespace voxedit {

bool ProtocolMessageFactory::isNewMessageAvailable(network::MessageStream &in) {
	in.seek(0, SEEK_SET);
	int32_t size = -1;
	in.peekInt32(size);
	if (size == -1) {
		in.seek(0, SEEK_END);
		// not enough data yet, wait a little bit more
		return false;
	}
	const int streamSize = static_cast<int>(in.size() - sizeof(int32_t));
	in.seek(0, SEEK_END);
	if (size > streamSize) {
		// not enough data yet, wait a little bit more
		return false;
	}
	return true;
}

network::ProtocolMessage *ProtocolMessageFactory::create(network::MessageStream &in) {
	in.seek(0);
	// remove the size from the stream
	uint32_t size = 0;
	in.readUInt32(size);
	// get the message type
	uint8_t type;
	in.readUInt8(type);
	Log::debug("Message of type %d with size %u", type, size);
	const int64_t startPos = in.pos();
	if (in.remaining() < size) {
		Log::error("Not enough data in the stream to read the full message. Remaining: %d, expected: %u",
				   (int)in.remaining(), size);
		return nullptr;
	}
	network::ProtocolMessage *msg = nullptr;
	switch (type) {
	case PROTO_PING:
		msg = new PingMessage();
		break;
	case PROTO_INIT_SESSION:
		msg = new InitSessionMessage(in);
		break;
	case PROTO_COMMAND:
		msg = new CommandMessage(in);
		break;
	case PROTO_SCENE_STATE_REQUEST:
		msg = new SceneStateRequestMessage();
		break;
	case PROTO_SCENE_STATE:
		msg = new SceneStateMessage(in, size);
		break;
	case PROTO_VOXEL_MODIFICATION:
		msg = new VoxelModificationMessage(in);
		break;
	case PROTO_NODE_ADDED:
		msg = new NodeAddedMessage(in);
		break;
	case PROTO_NODE_REMOVED:
		msg = new NodeRemovedMessage(in);
		break;
	case PROTO_NODE_MOVED:
		msg = new NodeMovedMessage(in);
		break;
	case PROTO_NODE_RENAMED:
		msg = new NodeRenamedMessage(in);
		break;
	case PROTO_NODE_PALETTE_CHANGED:
		msg = new NodePaletteChangedMessage(in);
		break;
	case PROTO_NODE_PROPERTIES:
		msg = new NodePropertiesMessage(in);
		break;
	case PROTO_NODE_KEYFRAMES:
		msg = new NodeKeyFramesMessage(in);
		break;
	case PROTO_NODE_NORMAL_PALETTE_CHANGED:
		msg = new NodeNormalPaletteChangedMessage(in);
		break;
	case PROTO_SCENE_GRAPH_ANIMATION:
		msg = new SceneGraphAnimationMessage(in);
		break;
	case PROTO_LUA_SCRIPTS_REQUEST:
		msg = new LuaScriptsRequestMessage();
		break;
	case PROTO_LUA_SCRIPTS_LIST:
		msg = new LuaScriptsListMessage(in);
		break;
	case PROTO_LUA_SCRIPT_CREATE:
		msg = new LuaScriptCreateMessage(in);
		break;
	default:
		Log::error("Unknown protocol message type: %u with size %u", type, size);
		break;
	}
	const int64_t endPos = in.pos();
	const uint32_t delta = endPos - startPos;
	if (size != delta) {
		Log::error("Message size mismatch: expected %u but read %u for message type %u", size, delta, type);
	}
	in.trim();
	in.seek(0, SEEK_END);
	return msg;
}


} // namespace voxedit
