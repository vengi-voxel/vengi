/**
 * @file
 */
#pragma once

#include "core/Assert.h"
#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Region.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {
namespace network {

using ProtocolId = uint8_t;
using MessageStream = io::BufferedReadWriteStream;

// keep alive message
const ProtocolId PROTO_PING = 0;
// request the initial scene state from the first client that connects
const ProtocolId PROTO_SCENE_STATE_REQUEST = 1;
// the complete scene state - the answer to the scene state request
const ProtocolId PROTO_SCENE_STATE = 2;
// voxel modification message with compressed voxel data that is broadcasted to all clients
const ProtocolId PROTO_VOXEL_MODIFICATION = 3;
// node added to the scene graph
const ProtocolId PROTO_NODE_ADDED = 4;
// node removed from the scene graph
const ProtocolId PROTO_NODE_REMOVED = 5;
// node moved in the scene graph to a new parent
const ProtocolId PROTO_NODE_MOVED = 6;
// node was renamed
const ProtocolId PROTO_NODE_RENAMED = 7;
// node palette was changed
const ProtocolId PROTO_NODE_PALETTE_CHANGED = 8;
// node properties were changed
const ProtocolId PROTO_NODE_PROPERTIES = 9;
// node key frames for a given animation were changed
const ProtocolId PROTO_NODE_KEYFRAMES = 10;
// initial session handshake message
const ProtocolId PROTO_INIT_SESSION = 11;

/**
 * @brief A protocol message is used for the serialization of the ai states for remote debugging
 */
class ProtocolMessage : public io::BufferedReadWriteStream {
protected:
	ProtocolId _id;

	void writeSize() {
		const int64_t bytes = pos();
		core_assert_always(bytes >= 5); // at least the size of the header
		core_assert_always(seek(0) != -1);
		core_assert_always(writeInt32(bytes - 5)); // 1 for the message type and 4 for the size itself
		core_assert_always(seek(0) != -1);
		Log::debug("Message size for type %d is %u", _id, (uint32_t)bytes);
	}

	void serializePalette(const palette::Palette &palette) {
		writePascalStringUInt16LE(palette.name());
		writeBool(palette.isBuiltIn());
		if (palette.isBuiltIn()) {
			return;
		}
		writeUInt8(palette.size());
		for (size_t i = 0; i < palette.size(); ++i) {
			const core::RGBA color = palette.color(i);
			writeUInt32(color.rgba);
			writePascalStringUInt16LE(palette.colorName(i));
			const palette::Material &material = palette.material(i);
			writeUInt8((uint8_t)material.type);
			writeUInt8(palette::MaterialProperty::MaterialMax - 1);
			for (uint32_t n = 0u; n < palette::MaterialProperty::MaterialMax - 1; ++n) {
				const char *materialName = palette::MaterialPropertyNames[n];
				writePascalStringUInt16LE(materialName);
				const palette::MaterialProperty property = (palette::MaterialProperty)(n + 1);
				const float value = material.value(property);
				writeFloat(value);
			}
		}
	}

	void deserializePalette(MessageStream &in, palette::Palette &palette) const {
		core::String paletteName;
		in.readPascalStringUInt16LE(paletteName);
		palette.setName(paletteName);
		bool isBuiltIn = in.readBool();
		if (isBuiltIn) {
			core_assert(palette::Palette::isBuiltIn(paletteName));
			palette.load(paletteName.c_str());
			return;
		}

		uint8_t paletteSize = 0;
		in.readUInt8(paletteSize);
		palette.setSize(paletteSize);
		for (uint8_t i = 0; i < paletteSize; ++i) {
			core::RGBA color;
			in.readUInt32(color.rgba);
			palette.setColor(i, color);
			core::String colorName;
			in.readPascalStringUInt16LE(colorName);
			palette.setColorName(i, colorName);

			// Read material type
			uint8_t materialType = 0;
			in.readUInt8(materialType);
			palette.setMaterialType(i, (palette::MaterialType)materialType);

			// Read material properties
			uint8_t propertyCount = 0;
			in.readUInt8(propertyCount);
			for (uint8_t n = 0; n < propertyCount; ++n) {
				core::String propertyName;
				in.readPascalStringUInt16LE(propertyName);
				float value;
				in.readFloat(value);
				palette.setMaterialProperty(i, propertyName, value);
			}
		}
	}

	bool serializeProperties(const scenegraph::SceneGraphNodeProperties &properties) {
		const size_t n = properties.size();
		if (!writeUInt16(n)) {
			Log::error("Failed to serialize properties");
			return false;
		}
		for (const auto &entry : properties) {
			if (!writePascalStringUInt16LE(entry->key)) {
				Log::error("Failed to serialize property key");
				return false;
			}
			if (!writePascalStringUInt16LE(entry->value)) {
				Log::error("Failed to serialize property value");
				return false;
			}
		}
		return true;
	}

	bool deserializeProperties(MessageStream &in, scenegraph::SceneGraphNodeProperties &properties) const {
		uint16_t propertyCount = 0;
		if (in.readUInt16(propertyCount) == -1) {
			Log::error("Failed to deserialize properties");
			return false;
		}
		for (uint16_t i = 0; i < propertyCount; ++i) {
			core::String key, value;
			if (!in.readPascalStringUInt16LE(key)) {
				Log::error("Failed to deserialize property key for property %d/%d", i, propertyCount);
				return false;
			}
			if (!in.readPascalStringUInt16LE(value)) {
				Log::error("Failed to deserialize property value for property %d/%d", i, propertyCount);
				return false;
			}
			properties.put(key, value);
		}
		return true;
	}

	void serializeKeyFrames(const scenegraph::SceneGraphKeyFramesMap &keyFrames) {
		writeUInt16(keyFrames.size());
		for (const auto &entry : keyFrames) {
			const core::String &animationName = entry->first;
			const scenegraph::SceneGraphKeyFrames &keyFramesList = entry->second;
			writePascalStringUInt16LE(animationName);
			writeUInt16(keyFramesList.size());
			for (const scenegraph::SceneGraphKeyFrame &keyFrame : keyFramesList) {
				writeInt32(keyFrame.frameIdx);
				writeBool(keyFrame.longRotation);
				writeUInt8((uint8_t)keyFrame.interpolation);
				// calculate it here, instead of using the getter, as the transform state
				// might still be dirty
				serializeMat4x4(keyFrame.transform().calculateLocalMatrix());
			}
		}
	}

	void deserializeKeyFrames(MessageStream &in, scenegraph::SceneGraphKeyFramesMap &keyFrames) const {
		uint16_t animationCount = 0;
		in.readUInt16(animationCount);
		for (uint16_t a = 0; a < animationCount; ++a) {
			core::String animationName;
			in.readPascalStringUInt16LE(animationName);
			uint16_t keyFrameCount;
			in.readUInt16(keyFrameCount);
			scenegraph::SceneGraphKeyFrames keyFramesList;
			for (uint16_t k = 0; k < keyFrameCount; ++k) {
				scenegraph::SceneGraphKeyFrame keyFrame;
				in.readInt32(keyFrame.frameIdx);
				keyFrame.longRotation = in.readBool();
				uint8_t interpolationValue;
				in.readUInt8(interpolationValue);
				keyFrame.interpolation = (scenegraph::InterpolationType)interpolationValue;
				glm::mat4 matrix;
				deserializeMat4x4(in, matrix);
				keyFrame.transform().setLocalMatrix(matrix);
				keyFramesList.push_back(keyFrame);
			}
			keyFrames.emplace(animationName, core::move(keyFramesList));
		}
	}

	bool serializeRegion(const voxel::Region &region) {
		core_assert_always(region.isValid());
		const glm::ivec3 &mins = region.getLowerCorner();
		const glm::ivec3 &maxs = region.getUpperCorner();
		if (!writeInt32(mins.x) || !writeInt32(mins.y) || !writeInt32(mins.z) || !writeInt32(maxs.x) ||
			!writeInt32(maxs.y) || !writeInt32(maxs.z)) {
			Log::error("Failed to serialize region");
			return false;
		}
		return true;
	}

	bool deserializeRegion(MessageStream &in, voxel::Region &region) const {
		glm::ivec3 mins{0}, maxs{0};
		if (in.readInt32(mins.x) == -1 || in.readInt32(mins.y) == -1 || in.readInt32(mins.z) == -1 ||
			in.readInt32(maxs.x) == -1 || in.readInt32(maxs.y) == -1 || in.readInt32(maxs.z) == -1) {
			Log::error("Failed to deserialize region");
			region = voxel::Region::InvalidRegion;
			return false;
		}
		region = voxel::Region{mins, maxs};
		core_assert_always(region.isValid());
		return true;
	}

	bool serializeMat4x4(const glm::mat4 &matrix) {
		const float *matrixPtr = glm::value_ptr(matrix);
		for (int i = 0; i < 16; ++i) {
			if (!writeFloat(matrixPtr[i])) {
				Log::error("Failed to serialize mat4x4");
				return false;
			}
		}
		return true;
	}

	bool deserializeMat4x4(MessageStream &in, glm::mat4 &matrix) const {
		float *matrixPtr = glm::value_ptr(matrix);
		for (int i = 0; i < 16; ++i) {
			if (in.readFloat(matrixPtr[i]) == -1) {
				Log::error("Failed to deserialize mat4x4");
				matrix = glm::mat4(1.0f);
				return false;
			}
		}
		return true;
	}

	bool serializeVec3(const glm::vec3 &vec) {
		if (!writeFloat(vec.x) || !writeFloat(vec.y) || !writeFloat(vec.z)) {
			Log::error("Failed to serialize vec3");
			return false;
		}
		return true;
	}

	bool deserializeVec3(MessageStream &in, glm::vec3 &vec) const {
		if (in.readFloat(vec.x) == -1 || in.readFloat(vec.y) == -1 || in.readFloat(vec.z) == -1) {
			Log::error("Failed to deserialize vec3");
			vec = glm::vec3(0.0f);
			return false;
		}
		return true;
	}

	void deserializeVolume(MessageStream &in, uint32_t &compressedSize, uint8_t *&compressedData) const {
		in.readUInt32(compressedSize);
		if (compressedSize > 0) {
			compressedData = new uint8_t[compressedSize];
			in.read((char *)compressedData, compressedSize);
		}
	}

	void serializeVolume(const uint8_t *compressedData, uint32_t compressedSize) {
		writeUInt32(compressedSize);
		write(compressedData, compressedSize);
	}
public:
	ProtocolMessage() : _id(0xff) {
	}
	explicit ProtocolMessage(const ProtocolId &id) : _id(id) {
		writeInt32(0);	 // size
		writeUInt8(_id); // message id

		// followed by data is contributing to the size - the header is not included in the size
	}

	virtual ~ProtocolMessage() {
	}

	virtual void writeBack() = 0;

	inline const ProtocolId &getId() const {
		return _id;
	}
};

#define PROTO_MSG(name, id)                                                                                            \
	class name : public voxedit::network::ProtocolMessage {                                                            \
	public:                                                                                                            \
		name() : voxedit::network::ProtocolMessage(id) {                                                               \
		}                                                                                                              \
		void writeBack() override {                                                                                    \
			writeInt32(0);                                                                                             \
			writeUInt8(_id);                                                                                           \
			writeSize();                                                                                               \
		}                                                                                                              \
	}

} // namespace network
} // namespace voxedit
