/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Scene graph node renamed message
 */
class NodeRenamedMessage : public network::ProtocolMessage {
private:
	core::UUID _nodeUUID;
	core::String _name;

public:
	NodeRenamedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_RENAMED) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodeRenamedMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(state.name)) {
			Log::error("Failed to write name in NodeRenamedMessage ctor");
			return;
		}
		writeSize();
	}
	/**
	 * @brief Construct a node renamed message with direct parameters
	 * @param nodeUUID The UUID of the node to rename
	 * @param name The new name for the node
	 */
	NodeRenamedMessage(const core::UUID &nodeUUID, const core::String &name) : ProtocolMessage(PROTO_NODE_RENAMED) {
		if (!writeUUID(nodeUUID)) {
			Log::error("Failed to write node UUID in NodeRenamedMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(name)) {
			Log::error("Failed to write name in NodeRenamedMessage ctor");
			return;
		}
		writeSize();
	}
	NodeRenamedMessage(network::MessageStream &in) {
		_id = PROTO_NODE_RENAMED;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for node renamed");
			return;
		}
		if (!in.readPascalStringUInt16LE(_name)) {
			Log::error("Failed to read name for node renamed %s", _nodeUUID.str().c_str());
			_name = "";
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodeRenamedMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in NodeRenamedMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_name)) {
			Log::error("Failed to write name in NodeRenamedMessage::writeBack");
			return;
		}
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


} // namespace voxedit
