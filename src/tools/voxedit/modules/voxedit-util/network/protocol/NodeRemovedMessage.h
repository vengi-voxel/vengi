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
		writeUUID(state.nodeUUID);
		writeSize();
	}
	NodeRemovedMessage(MessageStream &in) {
		_id = PROTO_NODE_REMOVED;
		in.readUUID(_nodeUUID);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUUID(_nodeUUID);
		writeSize();
	}
	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
};

} // namespace network
} // namespace voxedit
