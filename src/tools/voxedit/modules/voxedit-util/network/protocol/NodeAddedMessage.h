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
	core::String _parentUUID;
	core::String _nodeUUID;
	core::String _referenceUUID;
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
		writePascalStringUInt16LE(state.parentUUID);
		writePascalStringUInt16LE(state.nodeUUID);
		writePascalStringUInt16LE(state.referenceUUID);
		writePascalStringUInt16LE(state.name);
		writeUInt8((uint8_t)state.nodeType);
		serializeVec3(state.pivot);
		serializePalette(state.palette);
		serializeProperties(state.properties);
		if (state.nodeType == scenegraph::SceneGraphNodeType::Model) {
			serializeRegion(state.volumeRegion());
			serializeVolume(state.data.buffer(), state.data.size());
		}
		serializeKeyFrames(state.keyFrames);
		writeSize();
	}
	NodeAddedMessage(MessageStream &in) {
		_id = PROTO_NODE_ADDED;
		in.readPascalStringUInt16LE(_parentUUID);
		in.readPascalStringUInt16LE(_nodeUUID);
		in.readPascalStringUInt16LE(_referenceUUID);
		in.readPascalStringUInt16LE(_name);
		in.readUInt8(*(uint8_t *)&_nodeType);
		deserializeVec3(in, _pivot);
		deserializePalette(in, _palette);
		deserializeProperties(in, _properties);
		if (_nodeType == scenegraph::SceneGraphNodeType::Model) {
			deserializeRegion(in, _region);
			deserializeVolume(in, _compressedSize, _compressedData);
		}
		deserializeKeyFrames(in, _keyFrames);
	}
	~NodeAddedMessage() override {
		delete[] _compressedData;
	}

	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writePascalStringUInt16LE(_parentUUID);
		writePascalStringUInt16LE(_nodeUUID);
		writePascalStringUInt16LE(_referenceUUID);
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
	const core::String &parentUUID() const {
		return _parentUUID;
	}
	const core::String &nodeUUID() const {
		return _nodeUUID;
	}
	const core::String &referenceUUID() const {
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
