/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "voxedit-util/network/ProtocolMessage.h"

namespace voxedit {
namespace network {

/**
 * @brief Scene graph node removed message
 */
class NodeRemovedMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;

public:
	NodeRemovedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_REMOVED) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodeRemovedMessage ctor");
			return;
		}
		writeSize();
	}
	NodeRemovedMessage(MessageStream &in) {
		_id = PROTO_NODE_REMOVED;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for node removed");
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodeRemovedMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in NodeRemovedMessage::writeBack");
			return;
		}
		writeSize();
	}
	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
};

} // namespace network
} // namespace voxedit
