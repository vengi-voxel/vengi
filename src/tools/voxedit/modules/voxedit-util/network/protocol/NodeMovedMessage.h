/**
 * @file
 */

#pragma once

#include "memento/MementoHandler.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Scene graph node moved parent message
 */
class NodeMovedMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;
	core::UUID _parentUUID;
	core::UUID _referenceUUID;
	scenegraph::SceneGraphKeyFramesMap _keyFrames;

public:
	NodeMovedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_MOVED) {
		if (!writeUUID(state.nodeUUID) || !writeUUID(state.parentUUID) || !writeUUID(state.referenceUUID)) {
			Log::error("Failed to write UUIDs in NodeMovedMessage ctor");
			return;
		}
		if (!serializeKeyFrames(state.keyFrames)) {
			Log::error("Failed to write animation count in NodeMovedMessage ctor");
			return;
		}
		writeSize();
	}
	NodeMovedMessage(MessageStream &in) {
		_id = PROTO_NODE_MOVED;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for node moved");
			return;
		}
		if (in.readUUID(_parentUUID) == -1) {
			Log::error("Failed to read parent UUID for node moved");
			return;
		}
		if (in.readUUID(_referenceUUID) == -1) {
			Log::error("Failed to read reference UUID for node moved");
			return;
		}
		if (!deserializeKeyFrames(in, _keyFrames)) {
			Log::error("Failed to read key frames for node moved");
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodeMovedMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID) || !writeUUID(_parentUUID) || !writeUUID(_referenceUUID)) {
			Log::error("Failed to write UUIDs in NodeMovedMessage::writeBack");
			return;
		}
		if (!serializeKeyFrames(_keyFrames)) {
			Log::error("Failed to write animation count in NodeMovedMessage::writeBack");
			return;
		}
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
	const scenegraph::SceneGraphKeyFramesMap &keyFrames() const {
		return _keyFrames;
	}
};


} // namespace voxedit
