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
 * @brief Scene graph node moved parent message
 */
class NodeMovedMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;
	core::UUID _parentUUID;
	core::UUID _referenceUUID;

public:
	NodeMovedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_MOVED) {
		writeUUID(state.nodeUUID);
		writeUUID(state.parentUUID);
		writeUUID(state.referenceUUID);
		writeSize();
	}
	NodeMovedMessage(MessageStream &in) {
		_id = PROTO_NODE_MOVED;
		in.readUUID(_nodeUUID);
		in.readUUID(_parentUUID);
		in.readUUID(_referenceUUID);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUUID(_nodeUUID);
		writeUUID(_parentUUID);
		writeUUID(_referenceUUID);
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	/**
	 * @brief The new parent node uuid
	 */
	const core::UUID &parentUUID() const {
		return _parentUUID;
	}
	const core::UUID &referenceUUID() const {
		return _referenceUUID;
	}
};

} // namespace network
} // namespace voxedit
