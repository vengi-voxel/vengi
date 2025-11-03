/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Scene graph node properties changed message
 */
class NodePropertiesMessage : public network::ProtocolMessage {
private:
	core::UUID _nodeUUID;
	scenegraph::SceneGraphNodeProperties _properties;

public:
	NodePropertiesMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_PROPERTIES) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodePropertiesMessage ctor");
			return;
		}
		if (!serializeProperties(state.properties)) {
			Log::error("Failed to serialize properties in NodePropertiesMessage ctor");
			return;
		}
		writeSize();
	}
	NodePropertiesMessage(network::MessageStream &in) {
		_id = PROTO_NODE_PROPERTIES;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for node properties");
			return;
		}
		if (!deserializeProperties(in, _properties)) {
			Log::error("Failed to deserialize properties for node %s", _nodeUUID.str().c_str());
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodePropertiesMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in NodePropertiesMessage::writeBack");
			return;
		}
		if (!serializeProperties(_properties)) {
			Log::error("Failed to serialize properties in NodePropertiesMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	const scenegraph::SceneGraphNodeProperties &properties() const {
		return _properties;
	}
};


} // namespace voxedit
