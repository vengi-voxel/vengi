/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "core/String.h"
#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "voxedit-util/network/ProtocolMessage.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {
namespace network {

/**
 * @brief Scene graph node keyframes changed message
 */
class NodeKeyFramesMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;
	scenegraph::SceneGraphKeyFramesMap _keyFrames;

public:
	NodeKeyFramesMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_KEYFRAMES) {
		writeUUID(state.nodeUUID);
		writeUInt16(state.keyFrames.size());
		for (const auto &e : state.keyFrames) {
			const core::String &animation = e->first;
			const scenegraph::SceneGraphKeyFrames &keyFrames = e->second;
			writePascalStringUInt16LE(animation);
			writeUInt16(keyFrames.size());
			for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
				writeInt32(kf.frameIdx);
				writeBool(kf.longRotation);
				writeUInt8(core::enumVal(kf.interpolation));
				const scenegraph::SceneGraphTransform &transform = kf.transform();
				const glm::mat4 localMatrix = transform.calculateLocalMatrix();
				const float *localMatrixPtr = glm::value_ptr(localMatrix);
				for (int i = 0; i < 16; ++i) {
					writeFloat(localMatrixPtr[i]);
				}
			}
		}
		writeSize();
	}
	NodeKeyFramesMessage(MessageStream &in) {
		_id = PROTO_NODE_KEYFRAMES;
		in.readUUID(_nodeUUID);
		uint16_t keyFrameMapSize;
		in.readUInt16(keyFrameMapSize);
		for (uint16_t i = 0; i < keyFrameMapSize; ++i) {
			core::String animation;
			in.readPascalStringUInt16LE(animation);
			uint16_t keyFrameCount;
			in.readUInt16(keyFrameCount);
			scenegraph::SceneGraphKeyFrames keyFrames;
			keyFrames.reserve(keyFrameCount);
			for (uint16_t j = 0; j < keyFrameCount; ++j) {
				scenegraph::SceneGraphKeyFrame kf;
				in.readInt32(kf.frameIdx);
				kf.longRotation = in.readBool();
				uint8_t interpolationValue;
				in.readUInt8(interpolationValue);
				kf.interpolation = (scenegraph::InterpolationType)interpolationValue;
				glm::mat4 matrix;
				float *localMatrix = glm::value_ptr(matrix);
				for (int k = 0; k < 16; ++k) {
					in.readFloat(localMatrix[k]);
				}
				kf.transform().setLocalMatrix(matrix);
				keyFrames.push_back(kf);
			}
			_keyFrames.emplace(animation, core::move(keyFrames));
		}
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUUID(_nodeUUID);
		writeUInt16(_keyFrames.size());
		for (const auto &e : _keyFrames) {
			const core::String &animation = e->first;
			const scenegraph::SceneGraphKeyFrames &keyFrames = e->second;
			writePascalStringUInt16LE(animation);
			writeUInt16(keyFrames.size());
			for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
				writeInt32(kf.frameIdx);
				writeBool(kf.longRotation);
				writeUInt8(core::enumVal(kf.interpolation));
				const scenegraph::SceneGraphTransform &transform = kf.transform();
				const glm::mat4 localMatrix = transform.calculateLocalMatrix();
				const float *localMatrixPtr = glm::value_ptr(localMatrix);
				for (int i = 0; i < 16; ++i) {
					writeFloat(localMatrixPtr[i]);
				}
			}
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

} // namespace network
} // namespace voxedit
