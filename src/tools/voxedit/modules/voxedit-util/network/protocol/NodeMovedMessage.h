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
	core::String _nodeUUID;
	core::String _parentUUID;
	core::String _referenceUUID;

public:
	NodeMovedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_MOVED) {
		writePascalStringUInt16LE(state.nodeUUID);
		writePascalStringUInt16LE(state.parentUUID);
		writePascalStringUInt16LE(state.referenceUUID);
		writeSize();
	}
	NodeMovedMessage(MessageStream &in) {
		_id = PROTO_NODE_MOVED;
		in.readPascalStringUInt16LE(_nodeUUID);
		in.readPascalStringUInt16LE(_parentUUID);
		in.readPascalStringUInt16LE(_referenceUUID);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writePascalStringUInt16LE(_nodeUUID);
		writePascalStringUInt16LE(_parentUUID);
		writePascalStringUInt16LE(_referenceUUID);
		writeSize();
	}

	const core::String &nodeUUID() const {
		return _nodeUUID;
	}
	/**
	 * @brief The new parent node uuid
	 */
	const core::String &parentUUID() const {
		return _parentUUID;
	}
	const core::String &referenceUUID() const {
		return _referenceUUID;
	}
};

} // namespace network
} // namespace voxedit
