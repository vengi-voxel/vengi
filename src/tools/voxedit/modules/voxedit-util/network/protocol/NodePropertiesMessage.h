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
 * @brief Scene graph node properties changed message
 */
class NodePropertiesMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;
	scenegraph::SceneGraphNodeProperties _properties;

public:
	NodePropertiesMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_PROPERTIES) {
		writeUUID(state.nodeUUID);
		serializeProperties(state.properties);
		writeSize();
	}
	NodePropertiesMessage(MessageStream &in) {
		_id = PROTO_NODE_PROPERTIES;
		in.readUUID(_nodeUUID);
		deserializeProperties(in, _properties);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUUID(_nodeUUID);
		serializeProperties(_properties);
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	const scenegraph::SceneGraphNodeProperties &properties() const {
		return _properties;
	}
};

} // namespace network
} // namespace voxedit
