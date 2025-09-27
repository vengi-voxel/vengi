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
 * @brief Scene graph node renamed message
 */
class NodeRenamedMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;
	core::String _name;

public:
	NodeRenamedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_RENAMED) {
		writeUUID(state.nodeUUID);
		writePascalStringUInt16LE(state.name);
		writeSize();
	}
	NodeRenamedMessage(MessageStream &in) {
		_id = PROTO_NODE_RENAMED;
		in.readUUID(_nodeUUID);
		in.readPascalStringUInt16LE(_name);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUUID(_nodeUUID);
		writePascalStringUInt16LE(_name);
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	/**
	 * @brief The new name for the node
	 */
	const core::String &name() const {
		return _name;
	}
};

} // namespace network
} // namespace voxedit
