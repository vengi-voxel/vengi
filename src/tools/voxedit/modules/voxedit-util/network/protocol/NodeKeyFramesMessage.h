/**
 * @file
 */

#pragma once

#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "voxedit-util/network/ProtocolIds.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

/**
 * @brief Scene graph node keyframes changed message
 */
class NodeKeyFramesMessage : public network::ProtocolMessage {
private:
	core::UUID _nodeUUID;
	scenegraph::SceneGraphKeyFramesMap _keyFrames;

public:
	NodeKeyFramesMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_KEYFRAMES) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodeKeyFramesMessage ctor");
			return;
		}
		if (!serializeKeyFrames(state.keyFrames)) {
			Log::error("Failed to write animation count in NodeKeyFramesMessage ctor");
			return;
		}
		writeSize();
	}
	/**
	 * @brief Construct a node keyframes message with direct parameters
	 * @param nodeUUID The UUID of the node to update keyframes for
	 * @param keyFrames The keyframes map for the node
	 */
	NodeKeyFramesMessage(const core::UUID &nodeUUID, const scenegraph::SceneGraphKeyFramesMap &keyFrames)
		: ProtocolMessage(PROTO_NODE_KEYFRAMES) {
		if (!writeUUID(nodeUUID)) {
			Log::error("Failed to write node UUID in NodeKeyFramesMessage ctor");
			return;
		}
		if (!serializeKeyFrames(keyFrames)) {
			Log::error("Failed to write animation count in NodeKeyFramesMessage ctor");
			return;
		}
		writeSize();
	}
	NodeKeyFramesMessage(network::MessageStream &in) {
		_id = PROTO_NODE_KEYFRAMES;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for node keyframes");
			return;
		}
		if (!deserializeKeyFrames(in, _keyFrames)) {
			Log::error("Failed to read key frames for node keyframes");
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodeKeyFramesMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in NodeKeyFramesMessage::writeBack");
			return;
		}
		if (!serializeKeyFrames(_keyFrames)) {
			Log::error("Failed to write animation count in NodeKeyFramesMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	const scenegraph::SceneGraphKeyFramesMap &keyFrames() const {
		return _keyFrames;
	}
};

} // namespace voxedit
