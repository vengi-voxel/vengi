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
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodeKeyFramesMessage ctor");
			return;
		}
		if (!writeUInt16(state.keyFrames.size())) {
			Log::error("Failed to write animation count in NodeKeyFramesMessage ctor");
			return;
		}
		for (const auto &e : state.keyFrames) {
			const core::String &animation = e->first;
			const scenegraph::SceneGraphKeyFrames &keyFrames = e->second;
			if (!writePascalStringUInt16LE(animation)) {
				Log::error("Failed to write animation name in NodeKeyFramesMessage ctor");
				return;
			}
			if (!writeUInt16(keyFrames.size())) {
				Log::error("Failed to write key frame count in NodeKeyFramesMessage ctor");
				return;
			}
			for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
				if (!writeInt32(kf.frameIdx) || !writeBool(kf.longRotation) || !writeUInt8(core::enumVal(kf.interpolation))) {
					Log::error("Failed to write key frame header in NodeKeyFramesMessage ctor");
					return;
				}
				const scenegraph::SceneGraphTransform &transform = kf.transform();
				const glm::mat4 localMatrix = transform.calculateLocalMatrix();
				const float *localMatrixPtr = glm::value_ptr(localMatrix);
				for (int i = 0; i < 16; ++i) {
					if (!writeFloat(localMatrixPtr[i])) {
						Log::error("Failed to write key frame matrix in NodeKeyFramesMessage ctor");
						return;
					}
				}
			}
		}
		writeSize();
	}
	NodeKeyFramesMessage(MessageStream &in) {
		_id = PROTO_NODE_KEYFRAMES;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for node keyframes");
			return;
		}
		uint16_t keyFrameMapSize;
		if (in.readUInt16(keyFrameMapSize) == -1) {
			Log::error("Failed to read key frame map size");
			return;
		}
		for (uint16_t i = 0; i < keyFrameMapSize; ++i) {
			core::String animation;
			if (!in.readPascalStringUInt16LE(animation)) {
				Log::error("Failed to read animation name for node keyframes");
				return;
			}
			uint16_t keyFrameCount;
			if (in.readUInt16(keyFrameCount) == -1) {
				Log::error("Failed to read key frame count for animation %s", animation.c_str());
				return;
			}
			scenegraph::SceneGraphKeyFrames keyFrames;
			keyFrames.reserve(keyFrameCount);
			for (uint16_t j = 0; j < keyFrameCount; ++j) {
				scenegraph::SceneGraphKeyFrame kf;
				if (in.readInt32(kf.frameIdx) == -1) {
					Log::error("Failed to read frame index for animation %s", animation.c_str());
					return;
				}
				kf.longRotation = in.readBool();
				uint8_t interpolationValue;
				if (in.readUInt8(interpolationValue) == -1) {
					Log::error("Failed to read interpolation value for animation %s", animation.c_str());
					return;
				}
				kf.interpolation = (scenegraph::InterpolationType)interpolationValue;
				glm::mat4 matrix;
				float *localMatrix = glm::value_ptr(matrix);
				for (int k = 0; k < 16; ++k) {
					if (in.readFloat(localMatrix[k]) == -1) {
						Log::error("Failed to read keyframe matrix for animation %s", animation.c_str());
						return;
					}
				}
				kf.transform().setLocalMatrix(matrix);
				keyFrames.push_back(kf);
			}
			_keyFrames.emplace(animation, core::move(keyFrames));
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
		if (!writeUInt16(_keyFrames.size())) {
			Log::error("Failed to write animation count in NodeKeyFramesMessage::writeBack");
			return;
		}
		for (const auto &e : _keyFrames) {
			const core::String &animation = e->first;
			const scenegraph::SceneGraphKeyFrames &keyFrames = e->second;
			if (!writePascalStringUInt16LE(animation)) {
				Log::error("Failed to write animation name in NodeKeyFramesMessage::writeBack");
				return;
			}
			if (!writeUInt16(keyFrames.size())) {
				Log::error("Failed to write key frame count in NodeKeyFramesMessage::writeBack");
				return;
			}
			for (const scenegraph::SceneGraphKeyFrame &kf : keyFrames) {
				if (!writeInt32(kf.frameIdx) || !writeBool(kf.longRotation) || !writeUInt8(core::enumVal(kf.interpolation))) {
					Log::error("Failed to write key frame header in NodeKeyFramesMessage::writeBack");
					return;
				}
				const scenegraph::SceneGraphTransform &transform = kf.transform();
				const glm::mat4 localMatrix = transform.calculateLocalMatrix();
				const float *localMatrixPtr = glm::value_ptr(localMatrix);
				for (int i = 0; i < 16; ++i) {
					if (!writeFloat(localMatrixPtr[i])) {
						Log::error("Failed to write key frame matrix in NodeKeyFramesMessage::writeBack");
						return;
					}
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
