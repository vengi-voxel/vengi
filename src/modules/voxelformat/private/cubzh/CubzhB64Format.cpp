/**
 * @file
 */

#include "CubzhB64Format.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "io/Base64ReadStream.h"
#include <cstdint>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load b64 file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load b64 file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool CubzhB64Format::readChunkMap(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								  const palette::Palette &palette, const LoadContext &ctx) {
	double scale;
	wrap(stream.readDouble(scale)) // default is 5
	core::String name;
	wrapBool(stream.readPascalStringUInt32LE(name))
	Log::debug("map name: %s", name.c_str());
	return true;
}

bool CubzhB64Format::readAmbience(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								  const palette::Palette &palette, const LoadContext &ctx) {
	uint16_t size;
	wrap(stream.readUInt16(size))
	uint8_t nFields;
	wrap(stream.readUInt8(nFields))

	core::RGBA skyColor;
	core::RGBA skyHorizonColor;
	core::RGBA skyAbyssColor;
	core::RGBA skyLightColor;
	float skyLightIntensity;

	core::RGBA fogColor;
	float fogNear;
	float fogFar;
	float fogAbsorbtion;

	core::RGBA sunColor;
	float sunIntensity;
	float sunRotation[2];

	float ambientSkyLightFactor;
	float ambientDirLightFactor;

	core::String txt;

	for (uint16_t i = 0; i < nFields; ++i) {
		uint8_t fieldId[3];
		if (stream.read(fieldId, sizeof(fieldId)) != sizeof(fieldId)) {
			Log::error("Failed to read field Id");
			return false;
		}
		if (core_memcmp(fieldId, "ssc", 3) == 0) {
			wrap(stream.readUInt8(skyColor.r))
			wrap(stream.readUInt8(skyColor.g))
			wrap(stream.readUInt8(skyColor.b))
			wrap(stream.readUInt8(skyColor.a))
		} else if (core_memcmp(fieldId, "shc", 3) == 0) {
			wrap(stream.readUInt8(skyHorizonColor.r))
			wrap(stream.readUInt8(skyHorizonColor.g))
			wrap(stream.readUInt8(skyHorizonColor.b))
			wrap(stream.readUInt8(skyHorizonColor.a))
		} else if (core_memcmp(fieldId, "sac", 3) == 0) {
			wrap(stream.readUInt8(skyAbyssColor.r))
			wrap(stream.readUInt8(skyAbyssColor.g))
			wrap(stream.readUInt8(skyAbyssColor.b))
			wrap(stream.readUInt8(skyAbyssColor.a))
		} else if (core_memcmp(fieldId, "slc", 3) == 0) {
			wrap(stream.readUInt8(skyLightColor.r))
			wrap(stream.readUInt8(skyLightColor.g))
			wrap(stream.readUInt8(skyLightColor.b))
			wrap(stream.readUInt8(skyLightColor.a))
		} else if (core_memcmp(fieldId, "sli", 3) == 0) {
			wrap(stream.readFloat(skyLightIntensity));
		} else if (core_memcmp(fieldId, "foc", 3) == 0) {
			wrap(stream.readUInt8(fogColor.r))
			wrap(stream.readUInt8(fogColor.g))
			wrap(stream.readUInt8(fogColor.b))
			wrap(stream.readUInt8(fogColor.a))
		} else if (core_memcmp(fieldId, "fon", 3) == 0) {
			wrap(stream.readFloat(fogNear));
		} else if (core_memcmp(fieldId, "fof", 3) == 0) {
			wrap(stream.readFloat(fogFar));
		} else if (core_memcmp(fieldId, "foa", 3) == 0) {
			wrap(stream.readFloat(fogAbsorbtion));
		} else if (core_memcmp(fieldId, "suc", 3) == 0) {
			wrap(stream.readUInt8(sunColor.r))
			wrap(stream.readUInt8(sunColor.g))
			wrap(stream.readUInt8(sunColor.b))
			wrap(stream.readUInt8(sunColor.a))
		} else if (core_memcmp(fieldId, "sui", 3) == 0) {
			wrap(stream.readFloat(sunIntensity));
		} else if (core_memcmp(fieldId, "sur", 3) == 0) {
			wrap(stream.readFloat(sunRotation[0]));
			wrap(stream.readFloat(sunRotation[1]));
		} else if (core_memcmp(fieldId, "asl", 3) == 0) {
			wrap(stream.readFloat(ambientSkyLightFactor));
		} else if (core_memcmp(fieldId, "adl", 3) == 0) {
			wrap(stream.readFloat(ambientDirLightFactor));
		} else if (core_memcmp(fieldId, "txt", 3) == 0) {
			wrapBool(stream.readPascalStringUInt8(txt))
			Log::debug("ambience: txt: %s", txt.c_str());
		} else {
			Log::error("Unknown field id: %c%c%c", fieldId[0], fieldId[1], fieldId[2]);
			return false;
		}
	}

	return true;
}

bool CubzhB64Format::readBlocks(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								const palette::Palette &palette, const LoadContext &ctx) {
	uint32_t chunkSize;
	wrap(stream.readUInt32(chunkSize))
	Log::debug("chunkSize: %i", (int)chunkSize);
	uint16_t numBlocks;
	wrap(stream.readUInt16(numBlocks))
	Log::debug("numBlocks: %i", (int)numBlocks);
	for (uint16_t i = 0; i < numBlocks; ++i) {
		core::String key;
		wrapBool(stream.readPascalStringUInt16LE(key))
		Log::debug("key: %s", key.c_str());
		uint8_t blockAction;
		wrap(stream.readUInt8(blockAction))
		if (blockAction == 1) {
			core::RGBA color;
			wrap(stream.readUInt8(color.r))
			wrap(stream.readUInt8(color.g))
			wrap(stream.readUInt8(color.b))
			wrap(stream.readUInt8(color.a))
		}
	}
	return true;
}

#define CHECK_ID(field, id) core_memcmp((field), (id), 2) == 0

bool CubzhB64Format::readObjects(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								 const palette::Palette &palette, const LoadContext &ctx, int version) {
	if (version == 3) {
		uint32_t chunkLen;
		wrap(stream.readUInt32(chunkLen))
	} else {
		uint16_t chunkLen;
		wrap(stream.readUInt16(chunkLen))
	}
	uint16_t numObjects;
	wrap(stream.readUInt16(numObjects))
	Log::debug("numObjects: %i", numObjects);
	uint16_t instanceCount = 0;
	while (instanceCount < numObjects) {
		core::String name;
		wrapBool(stream.readPascalStringUInt16LE(name))
		uint16_t numInstances;
		wrap(stream.readUInt16(numInstances))
		Log::debug("numInstances: %i, instanceCount: %i, numObjects: %i",
				numInstances, instanceCount, numObjects);
		for (uint16_t j = 0; j < numInstances; ++j) {
			uint8_t numFields;
			wrap(stream.readUInt8(numFields))
			core::String uuid;
			core::String name;
			glm::vec3 pos{0.0f};
			glm::vec3 rot{0.0f};
			glm::vec3 scale{1.0f};
			uint8_t physicMode = 0;
			Log::debug("numFields: %i", numFields);
			for (uint8_t k = 0; k < numFields; ++k) {
				uint8_t fieldId[2];
				if (stream.read(fieldId, sizeof(fieldId)) != sizeof(fieldId)) {
					Log::error("Failed to read object field Id");
					return false;
				}
				Log::debug("%c%c", fieldId[0], fieldId[1]);
				if (CHECK_ID(fieldId, "id")) {
					wrapBool(stream.readPascalStringUInt8(uuid))
				} else if (CHECK_ID(fieldId, "po")) {
					wrap(stream.readFloat(pos.x))
					wrap(stream.readFloat(pos.y))
					wrap(stream.readFloat(pos.z))
				} else if (CHECK_ID(fieldId, "ro")) {
					wrap(stream.readFloat(rot.x))
					wrap(stream.readFloat(rot.y))
					wrap(stream.readFloat(rot.z))
				} else if (CHECK_ID(fieldId, "sc")) {
					wrap(stream.readFloat(scale.x))
					wrap(stream.readFloat(scale.y))
					wrap(stream.readFloat(scale.z))
				} else if (CHECK_ID(fieldId, "na")) {
					wrapBool(stream.readPascalStringUInt8(name))
				} else if (CHECK_ID(fieldId, "de")) {
					core_assert(version == 2);
					core::String base64;
					// itemDetailsCell table
					wrapBool(stream.readPascalStringUInt16LE(base64))
					// TODO
				} else if (CHECK_ID(fieldId, "pm")) {
					wrap(stream.readUInt8(physicMode))
				} else {
					Log::error("Unknown field id: %c%c", fieldId[0], fieldId[1]);
					return false;
				}
			}
			++instanceCount;
			Log::debug("Object: %s", name.c_str());
			Log::debug("UUID: %s", uuid.c_str());
			Log::debug("Position: %f %f %f", pos.x, pos.y, pos.z);
			Log::debug("Rotation: %f %f %f", rot.x, rot.y, rot.z);
			Log::debug("Scale: %f %f %f", scale.x, scale.y, scale.z);
			Log::debug("Physic mode: %i", (int)physicMode);
		}
	}
	return true;
}

bool CubzhB64Format::loadVersion1(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								  const palette::Palette &palette, const LoadContext &ctx) {
	// TODO: not supported - base64 lua tables
	uint8_t chunkId;
	wrap(stream.readUInt8(chunkId))
	wrapBool(readChunkMap(stream, sceneGraph, palette, ctx))
	wrap(stream.readUInt8(chunkId))
	wrapBool(readAmbience(stream, sceneGraph, palette, ctx))
	wrap(stream.readUInt8(chunkId))
	wrapBool(readBlocks(stream, sceneGraph, palette, ctx))
	return false;
}

bool CubzhB64Format::loadVersion2(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								  const palette::Palette &palette, const LoadContext &ctx) {
	while (!stream.eos()) {
		uint8_t chunkId;
		wrap(stream.readUInt8(chunkId))
		switch (chunkId) {
		case 0:
			wrapBool(readChunkMap(stream, sceneGraph, palette, ctx))
			break;
		case 1:
			wrapBool(readAmbience(stream, sceneGraph, palette, ctx))
			break;
		case 2:
			wrapBool(readObjects(stream, sceneGraph, palette, ctx, 2))
			break;
		case 3:
			wrapBool(readBlocks(stream, sceneGraph, palette, ctx))
			break;
		default:
			Log::error("Unknown chunk id: %i", (int)chunkId);
			return false;
		}
	}
	return true;
}

bool CubzhB64Format::loadVersion3(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								  const palette::Palette &palette, const LoadContext &ctx) {
	while (!stream.eos()) {
		uint8_t chunkId;
		wrap(stream.readUInt8(chunkId))
		Log::debug("chunk id: %u", chunkId);
		switch (chunkId) {
		case 0:
			wrapBool(readChunkMap(stream, sceneGraph, palette, ctx))
			break;
		case 1:
			wrapBool(readAmbience(stream, sceneGraph, palette, ctx))
			break;
		case 2:
			wrapBool(readObjects(stream, sceneGraph, palette, ctx, 3))
			break;
		case 3:
			wrapBool(readBlocks(stream, sceneGraph, palette, ctx))
			break;
		default:
			Log::error("Unknown chunk id: %i", (int)chunkId);
			return false;
		}
	}
	return true;
}

bool CubzhB64Format::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
									scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
									const LoadContext &ctx) {
	io::Base64ReadStream base64Stream(stream);
	uint8_t version;
	wrap(base64Stream.readUInt8(version))
	if (version == 1) {
		return loadVersion1(base64Stream, sceneGraph, palette, ctx);
	} else if (version == 2) {
		return loadVersion2(base64Stream, sceneGraph, palette, ctx);
	} else if (version == 3) {
		return loadVersion3(base64Stream, sceneGraph, palette, ctx);
	}

	Log::error("Unsupported version found: %i", (int)version);
	return false;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
