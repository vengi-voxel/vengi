/**
 * @file
 */
#pragma once

#include "core/Assert.h"
#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Region.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

using ProtocolId = uint8_t;
using MessageStream = io::BufferedReadWriteStream;

class ProtocolMessage : public io::BufferedReadWriteStream {
protected:
	ProtocolId _id;

	class ScopedProtocolMessageSerialization {
	private:
		ProtocolMessage *_msg;
		int64_t _startPos;
		const char *_type;
	public:
		ScopedProtocolMessageSerialization(ProtocolMessage *msg, const char *type) : _msg(msg), _type(type) {
			_startPos = _msg->pos();
		}
		~ScopedProtocolMessageSerialization() {
			const int64_t endPos = _msg->pos();
			const uint32_t delta = endPos - _startPos;
			Log::debug("Serialized %u bytes for message type %d (%s)", delta, _msg->_id, _type);
		}
	};

	class ScopedProtocolStreamDeserialization {
	private:
		MessageStream *_msg;
		int64_t _startPos;
		const char *_type;
	public:
		ScopedProtocolStreamDeserialization(MessageStream &msg, const char *type) : _msg(&msg), _type(type) {
			_startPos = _msg->pos();
		}
		~ScopedProtocolStreamDeserialization() {
			const int64_t endPos = _msg->pos();
			const uint32_t delta = endPos - _startPos;
			Log::debug("Deserialized %u bytes for type %s", delta, _type);
		}
	};

	void writeSize() {
		const int64_t bytes = pos();
		core_assert_always(bytes >= 5); // at least the size of the header
		core_assert_always(seek(0) != -1);
		core_assert_always(writeInt32(bytes - 5)); // 1 for the message type and 4 for the size itself
		core_assert_always(seek(0) != -1);
		Log::debug("Message size for type %d is %u", _id, (uint32_t)bytes);
	}

	bool serializePalette(const palette::Palette &palette) {
		ScopedProtocolMessageSerialization s(this, "Palette");
		if (!writePascalStringUInt16LE(palette.name())) {
			Log::error("Failed to write palette name");
			return false;
		}
		if (!writeBool(palette.isBuiltIn())) {
			Log::error("Failed to write built-in flag");
			return false;
		}
		if (palette.isBuiltIn()) {
			return true;
		}
		if (!writeUInt16(palette.colorCount())) {
			Log::error("Failed to write palette size");
			return false;
		}
		for (int i = 0; i < palette.colorCount(); ++i) {
			const core::RGBA color = palette.color(i);
			const palette::Material &material = palette.material(i);
			if (!writeUInt32(color.rgba)) {
				Log::error("Failed to write color for palette index %d/%d", (int)i, (int)palette.size());
				return false;
			}
			if (!writePascalStringUInt16LE(palette.colorName(i))) {
				Log::error("Failed to write color name for palette index %d/%d", (int)i, (int)palette.size());
				return false;
			}
			if (!writeUInt8((uint8_t)material.type)) {
				Log::error("Failed to write material type for palette index %d/%d", (int)i, (int)palette.size());
				return false;
			}
			if (!writeUInt8(palette::MaterialProperty::MaterialMax - 1)) {
				Log::error("Failed to write material property count for palette index %d/%d", (int)i,
						   (int)palette.size());
				return false;
			}
			for (uint32_t n = 0u; n < palette::MaterialProperty::MaterialMax - 1; ++n) {
				const char *materialName = palette::MaterialPropertyNames[n];
				if (!writePascalStringUInt16LE(materialName)) {
					Log::error("Failed to write material property name");
					return false;
				}
				const palette::MaterialProperty property = (palette::MaterialProperty)(n + 1);
				const float value = material.value(property);
				if (!writeFloat(value)) {
					Log::error("Failed to write material property value");
					return false;
				}
			}
		}
		return true;
	}

	static bool deserializePalette(MessageStream &in, palette::Palette &palette) {
		ScopedProtocolStreamDeserialization s(in, "Palette");
		core::String paletteName;
		if (!in.readPascalStringUInt16LE(paletteName)) {
			Log::error("Failed to read palette name");
			return false;
		}
		palette.setName(paletteName);
		bool isBuiltIn = in.readBool();
		if (isBuiltIn) {
			Log::debug("Built-in palette - nothing more to read");
			core_assert(palette::Palette::isBuiltIn(paletteName));
			return palette.load(paletteName.c_str());
		}

		uint16_t paletteSize = 0;
		if (in.readUInt16(paletteSize) == -1) {
			Log::error("Failed to read palette size");
			return false;
		}
		palette.setSize(paletteSize);
		for (uint16_t i = 0; i < paletteSize; ++i) {
			core::RGBA color;
			if (in.readUInt32(color.rgba) == -1) {
				Log::error("Failed to read color for palette index %d/%d", i, paletteSize);
				return false;
			}
			palette.setColor(i, color);
			core::String colorName;
			if (!in.readPascalStringUInt16LE(colorName)) {
				Log::error("Failed to read color name for palette index %d/%d", i, paletteSize);
				return false;
			}
			palette.setColorName(i, colorName);

			// Read material type
			uint8_t materialType = 0;
			if (in.readUInt8(materialType) == -1) {
				Log::error("Failed to read material type for palette index %d/%d", i, paletteSize);
				return false;
			}
			palette.setMaterialType(i, (palette::MaterialType)materialType);

			// Read material properties
			uint8_t propertyCount = 0;
			if (in.readUInt8(propertyCount) == -1) {
				Log::error("Failed to read material property count for palette index %d/%d", i, paletteSize);
				return false;
			}
			for (uint8_t n = 0; n < propertyCount; ++n) {
				core::String propertyName;
				if (!in.readPascalStringUInt16LE(propertyName)) {
					Log::error("Failed to read material property name for palette index %d/%d", i, paletteSize);
					return false;
				}
				float value;
				if (in.readFloat(value) == -1) {
					Log::error("Failed to read material property value for palette index %d/%d", i, paletteSize);
					return false;
				}
				palette.setMaterialProperty(i, propertyName, value);
			}
		}
		return true;
	}

	bool serializeProperties(const scenegraph::SceneGraphNodeProperties &properties) {
		ScopedProtocolMessageSerialization s(this, "Properties");
		const uint16_t n = (uint16_t)properties.size();
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

	static bool deserializeProperties(MessageStream &in, scenegraph::SceneGraphNodeProperties &properties) {
		ScopedProtocolStreamDeserialization s(in, "Properties");
		uint16_t propertyCount = 0;
		if (in.readUInt16(propertyCount) == -1) {
			Log::error("Failed to deserialize properties");
			return false;
		}
		for (uint16_t i = 0; i < propertyCount; ++i) {
			core::String key, value;
			if (!in.readPascalStringUInt16LE(key)) {
				Log::error("Failed to deserialize property key for property %d/%d", (int)i, (int)propertyCount);
				return false;
			}
			if (!in.readPascalStringUInt16LE(value)) {
				Log::error("Failed to deserialize property value for property %d/%d", (int)i, (int)propertyCount);
				return false;
			}
			properties.put(key, value);
		}
		return true;
	}

	bool serializeKeyFrames(const scenegraph::SceneGraphKeyFramesMap &keyFrames) {
		ScopedProtocolMessageSerialization s(this, "Keyframes");
		if (!writeUInt16((uint16_t)keyFrames.size())) {
			Log::error("Failed to serialize key frame animation count");
			return false;
		}
		for (const auto &entry : keyFrames) {
			const core::String &animationName = entry->first;
			const scenegraph::SceneGraphKeyFrames &keyFramesList = entry->second;
			if (!writePascalStringUInt16LE(animationName)) {
				Log::error("Failed to serialize animation name");
				return false;
			}
			if (!writeUInt16(keyFramesList.size())) {
				Log::error("Failed to serialize key frame count");
				return false;
			}
			for (const scenegraph::SceneGraphKeyFrame &keyFrame : keyFramesList) {
				if (!writeInt32(keyFrame.frameIdx)) {
					Log::error("Failed to serialize frame index");
					return false;
				}
				if (!writeBool(keyFrame.longRotation)) {
					Log::error("Failed to serialize long rotation");
					return false;
				}
				if (!writeUInt8((uint8_t)keyFrame.interpolation)) {
					Log::error("Failed to serialize interpolation");
					return false;
				}
				// calculate it here, instead of using the getter, as the transform state
				// might still be dirty
				if (!serializeMat4x4(keyFrame.transform().calculateLocalMatrix())) {
					return false;
				}
			}
		}
		return true;
	}

	static bool deserializeKeyFrames(MessageStream &in, scenegraph::SceneGraphKeyFramesMap &keyFrames) {
		ScopedProtocolStreamDeserialization s(in, "Keyfranes");
		uint16_t animationCount = 0;
		if (in.readUInt16(animationCount) == -1) {
			Log::error("Failed to read animation count");
			return false;
		}
		for (uint16_t a = 0; a < animationCount; ++a) {
			core::String animationName;
			if (!in.readPascalStringUInt16LE(animationName)) {
				Log::error("Failed to read animation name for animation %d/%d", a, animationCount);
				return false;
			}
			uint16_t keyFrameCount;
			if (in.readUInt16(keyFrameCount) == -1) {
				Log::error("Failed to read key frame count for animation %s", animationName.c_str());
				return false;
			}
			scenegraph::SceneGraphKeyFrames keyFramesList;
			for (uint16_t k = 0; k < keyFrameCount; ++k) {
				scenegraph::SceneGraphKeyFrame keyFrame;
				if (in.readInt32(keyFrame.frameIdx) == -1) {
					Log::error("Failed to read frame index");
					return false;
				}
				keyFrame.longRotation = in.readBool();
				uint8_t interpolationValue;
				if (in.readUInt8(interpolationValue) == -1) {
					Log::error("Failed to read interpolation value");
					return false;
				}
				keyFrame.interpolation = (scenegraph::InterpolationType)interpolationValue;
				glm::mat4 matrix;
				if (!deserializeMat4x4(in, matrix)) {
					return false;
				}
				keyFrame.transform().setLocalMatrix(matrix);
				keyFramesList.push_back(keyFrame);
			}
			keyFrames.emplace(animationName, core::move(keyFramesList));
		}
		return true;
	}

	bool serializeRegion(const voxel::Region &region) {
		ScopedProtocolMessageSerialization s(this, "Region");
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

	static bool deserializeRegion(MessageStream &in, voxel::Region &region) {
		ScopedProtocolStreamDeserialization s(in, "Region");
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
		ScopedProtocolMessageSerialization s(this, "Mat4x4");
		const float *matrixPtr = glm::value_ptr(matrix);
		for (int i = 0; i < 16; ++i) {
			if (!writeFloat(matrixPtr[i])) {
				Log::error("Failed to serialize mat4x4");
				return false;
			}
		}
		return true;
	}

	static bool deserializeMat4x4(MessageStream &in, glm::mat4 &matrix) {
		ScopedProtocolStreamDeserialization s(in, "Mat4x4");
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
		ScopedProtocolMessageSerialization s(this, "Vec3");
		if (!writeFloat(vec.x) || !writeFloat(vec.y) || !writeFloat(vec.z)) {
			Log::error("Failed to serialize vec3");
			return false;
		}
		return true;
	}

	static bool deserializeVec3(MessageStream &in, glm::vec3 &vec) {
		ScopedProtocolStreamDeserialization s(in, "Vec3");
		if (in.readFloat(vec.x) == -1 || in.readFloat(vec.y) == -1 || in.readFloat(vec.z) == -1) {
			Log::error("Failed to deserialize vec3");
			vec = glm::vec3(0.0f);
			return false;
		}
		return true;
	}

	bool serializeVolume(const uint8_t *compressedData, uint32_t compressedSize) {
		ScopedProtocolMessageSerialization s(this, "Volume");
		if (!writeUInt32(compressedSize)) {
			Log::error("Failed to write compressed size");
			return false;
		}
		if (write(compressedData, compressedSize) == -1) {
			Log::error("Failed to write compressed volume data");
			return false;
		}
		return true;
	}

	static bool deserializeVolume(MessageStream &in, uint32_t &compressedSize, uint8_t *&compressedData) {
		ScopedProtocolStreamDeserialization s(in, "Volume");
		if (in.readUInt32(compressedSize) == -1) {
			compressedSize = 0;
			compressedData = nullptr;
			Log::error("Failed to read compressed size");
			return false;
		}
		if (compressedSize > 0) {
			compressedData = new uint8_t[compressedSize];
			if (in.read((char *)compressedData, compressedSize) == -1) {
				Log::error("Failed to read compressed volume data");
				delete[] compressedData;
				compressedData = nullptr;
				compressedSize = 0;
				return false;
			}
		}
		return true;
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

	// write a deserialized message back into the byte stream to sending over the wire
	virtual void writeBack() = 0;

	inline const ProtocolId &getId() const {
		return _id;
	}
};

#define PROTO_MSG(name, id)                                                                                            \
	class name : public voxedit::ProtocolMessage {                                                            \
	public:                                                                                                            \
		name() : voxedit::ProtocolMessage(id) {                                                               \
		}                                                                                                              \
		void writeBack() override {                                                                                    \
			writeInt32(0);                                                                                             \
			writeUInt8(_id);                                                                                           \
			writeSize();                                                                                               \
		}                                                                                                              \
	}

} // namespace voxedit
