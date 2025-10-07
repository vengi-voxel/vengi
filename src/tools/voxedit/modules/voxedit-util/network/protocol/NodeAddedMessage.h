/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/network/ProtocolMessage.h"

namespace voxedit {
namespace network {

/**
 * @brief Scene graph node added message
 */
class NodeAddedMessage : public ProtocolMessage {
private:
	core::UUID _parentUUID;
	core::UUID _nodeUUID;
	core::UUID _referenceUUID;
	core::String _name;
	scenegraph::SceneGraphNodeType _nodeType = scenegraph::SceneGraphNodeType::Unknown;
	glm::vec3 _pivot{0.0f};
	palette::Palette _palette;
	scenegraph::SceneGraphNodeProperties _properties;
	scenegraph::SceneGraphKeyFramesMap _keyFrames;
	uint32_t _compressedSize = 0;
	uint8_t *_compressedData = nullptr;
	voxel::Region _region;

public:
	NodeAddedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_ADDED) {
		if (!writeUUID(state.parentUUID)) {
			Log::error("Failed to write parent UUID");
			return;
		}
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID");
			return;
		}
		if (!writeUUID(state.referenceUUID)) {
			Log::error("Failed to write reference UUID");
			return;
		}
		if (!writePascalStringUInt16LE(state.name)) {
			Log::error("Failed to write node name");
			return;
		}
		if (!writeUInt8((uint8_t)state.nodeType)) {
			Log::error("Failed to write node type");
			return;
		}
		if (!serializeVec3(state.pivot)) {
			return;
		}
		if (!serializePalette(state.palette)) {
			return;
		}
		if (!serializeProperties(state.properties)) {
			return;
		}
		if (state.nodeType == scenegraph::SceneGraphNodeType::Model) {
			if (!serializeRegion(state.volumeRegion())) {
				return;
			}
			if (!serializeVolume(state.data.buffer(), state.data.size())) {
				return;
			}
		}
		if (!serializeKeyFrames(state.keyFrames)) {
			return;
		}
		writeSize();
	}
	NodeAddedMessage(MessageStream &in) {
		_id = PROTO_NODE_ADDED;
		if (in.readUUID(_parentUUID) == -1) {
			Log::error("Failed to read parent UUID");
		}
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID");
		}
		if (in.readUUID(_referenceUUID) == -1) {
			Log::error("Failed to read reference UUID");
		}
		if (!in.readPascalStringUInt16LE(_name)) {
			Log::error("Failed to read node name");
			_name = "";
		}
		in.readUInt8(*(uint8_t *)&_nodeType);
		if (!deserializeVec3(in, _pivot)) {
			Log::error("Failed to deserialize pivot for node: %s (%s)", _name.c_str(), _nodeUUID.str().c_str());
		}
		if (!deserializePalette(in, _palette)) {
			Log::error("Failed to deserialize palette for node: %s (%s)", _name.c_str(), _nodeUUID.str().c_str());
		}
		if (!deserializeProperties(in, _properties)) {
			Log::error("Failed to deserialize properties for node: %s (%s)", _name.c_str(), _nodeUUID.str().c_str());
		}
		if (_nodeType == scenegraph::SceneGraphNodeType::Model) {
			if (!deserializeRegion(in, _region)) {
				Log::error("Failed to deserialize region for node: %s (%s)", _name.c_str(), _nodeUUID.str().c_str());
			}
			if (!deserializeVolume(in, _compressedSize, _compressedData)) {
				Log::error("Failed to deserialize volume for node: %s (%s)", _name.c_str(), _nodeUUID.str().c_str());
			}
		}
		if (!deserializeKeyFrames(in, _keyFrames)) {
			Log::error("Failed to deserialize key frames for node: %s (%s)", _name.c_str(), _nodeUUID.str().c_str());
		}
	}
	~NodeAddedMessage() override {
		delete[] _compressedData;
	}

	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUUID(_parentUUID);
		writeUUID(_nodeUUID);
		writeUUID(_referenceUUID);
		writePascalStringUInt16LE(_name);
		writeUInt8((uint8_t)_nodeType);
		serializeVec3(_pivot);
		serializePalette(_palette);
		serializeProperties(_properties);
		if (_nodeType == scenegraph::SceneGraphNodeType::Model) {
			serializeRegion(_region);
			serializeVolume(_compressedData, _compressedSize);
		}
		serializeKeyFrames(_keyFrames);
		writeSize();
	}
	const core::UUID &parentUUID() const {
		return _parentUUID;
	}
	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	const core::UUID &referenceUUID() const {
		return _referenceUUID;
	}
	const core::String &name() const {
		return _name;
	}
	scenegraph::SceneGraphNodeType nodeType() const {
		return _nodeType;
	}
	const glm::vec3 &pivot() const {
		return _pivot;
	}
	const palette::Palette &palette() const {
		return _palette;
	}
	const scenegraph::SceneGraphNodeProperties &properties() const {
		return _properties;
	}
	const scenegraph::SceneGraphKeyFramesMap &keyFrames() const {
		return _keyFrames;
	}
	uint32_t compressedSize() const {
		return _compressedSize;
	}
	const uint8_t *compressedData() const {
		return _compressedData;
	}
	const voxel::Region &region() const {
		return _region;
	}
};

} // namespace network
} // namespace voxedit
