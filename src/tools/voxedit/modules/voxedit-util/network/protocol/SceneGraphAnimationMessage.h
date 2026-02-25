/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Scene graph animation list changed message
 */
class SceneGraphAnimationMessage : public network::ProtocolMessage {
private:
	core::DynamicArray<core::String> _animations;

public:
	SceneGraphAnimationMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_SCENE_GRAPH_ANIMATION) {
		if (state.stringList.hasValue()) {
			const auto* list = state.stringList.value();
			_animations.reserve(list->size());
			for (const auto& s : *list) {
				_animations.push_back(s);
			}
		}
		if (!writeUInt16((uint16_t)_animations.size())) {
			Log::error("Failed to write animation count in SceneGraphAnimationMessage ctor");
			return;
		}
		for (const core::String &anim : _animations) {
			if (!writePascalStringUInt16LE(anim)) {
				Log::error("Failed to write animation name in SceneGraphAnimationMessage ctor");
				return;
			}
		}
		writeSize();
	}

	/**
	 * @brief Construct an animation message with direct parameters
	 * @param animations The list of animation names
	 */
	SceneGraphAnimationMessage(const core::DynamicArray<core::String> &animations)
		: ProtocolMessage(PROTO_SCENE_GRAPH_ANIMATION), _animations(animations) {
		if (!writeUInt16((uint16_t)_animations.size())) {
			Log::error("Failed to write animation count in SceneGraphAnimationMessage ctor");
			return;
		}
		for (const core::String &anim : _animations) {
			if (!writePascalStringUInt16LE(anim)) {
				Log::error("Failed to write animation name in SceneGraphAnimationMessage ctor");
				return;
			}
		}
		writeSize();
	}

	SceneGraphAnimationMessage(network::MessageStream &in) {
		_id = PROTO_SCENE_GRAPH_ANIMATION;
		uint16_t count = 0;
		if (in.readUInt16(count) == -1) {
			Log::error("Failed to read animation count");
			return;
		}
		_animations.reserve(count);
		for (uint16_t i = 0; i < count; ++i) {
			core::String anim;
			if (!in.readPascalStringUInt16LE(anim)) {
				Log::error("Failed to read animation name");
				return;
			}
			_animations.push_back(anim);
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in SceneGraphAnimationMessage::writeBack");
			return;
		}
		if (!writeUInt16((uint16_t)_animations.size())) {
			Log::error("Failed to write animation count in SceneGraphAnimationMessage::writeBack");
			return;
		}
		for (const core::String &anim : _animations) {
			if (!writePascalStringUInt16LE(anim)) {
				Log::error("Failed to write animation name in SceneGraphAnimationMessage::writeBack");
				return;
			}
		}
		writeSize();
	}

	const core::DynamicArray<core::String> &animations() const {
		return _animations;
	}
};

} // namespace voxedit
