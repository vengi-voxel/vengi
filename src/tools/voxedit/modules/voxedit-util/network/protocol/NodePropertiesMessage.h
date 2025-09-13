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
	core::String _nodeUUID;
	scenegraph::SceneGraphNodeProperties _properties;

public:
	NodePropertiesMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_PROPERTIES) {
		writePascalStringUInt16LE(state.nodeUUID);
		serializeProperties(state.properties);
		writeSize();
	}
	NodePropertiesMessage(MessageStream &in) {
		_id = PROTO_NODE_PROPERTIES;
		in.readPascalStringUInt16LE(_nodeUUID);
		deserializeProperties(in, _properties);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writePascalStringUInt16LE(_nodeUUID);
		serializeProperties(_properties);
		writeSize();
	}

	const core::String &nodeUUID() const {
		return _nodeUUID;
	}
	const scenegraph::SceneGraphNodeProperties &properties() const {
		return _properties;
	}
};

} // namespace network
} // namespace voxedit
