/**
 * @file
 */

#include "CubzhB64Format.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "io/Base64ReadStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/FilesystemArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/cubzh/CubzhFormat.h"
#include <stdint.h>
#include <glm/gtx/euler_angles.hpp>

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
								  const palette::Palette &palette, const LoadContext &ctx, Ambience &ambience) {
	uint16_t size;
	wrap(stream.readUInt16(size))
	uint8_t nFields;
	wrap(stream.readUInt8(nFields))

	for (uint16_t i = 0; i < nFields; ++i) {
		uint8_t fieldId[3];
		if (stream.read(fieldId, sizeof(fieldId)) != sizeof(fieldId)) {
			Log::error("Failed to read field Id");
			return false;
		}
		if (core_memcmp(fieldId, "ssc", 3) == 0) {
			wrap(stream.readUInt8(ambience.skyColor.r))
			wrap(stream.readUInt8(ambience.skyColor.g))
			wrap(stream.readUInt8(ambience.skyColor.b))
			wrap(stream.readUInt8(ambience.skyColor.a))
		} else if (core_memcmp(fieldId, "shc", 3) == 0) {
			wrap(stream.readUInt8(ambience.skyHorizonColor.r))
			wrap(stream.readUInt8(ambience.skyHorizonColor.g))
			wrap(stream.readUInt8(ambience.skyHorizonColor.b))
			wrap(stream.readUInt8(ambience.skyHorizonColor.a))
		} else if (core_memcmp(fieldId, "sac", 3) == 0) {
			wrap(stream.readUInt8(ambience.skyAbyssColor.r))
			wrap(stream.readUInt8(ambience.skyAbyssColor.g))
			wrap(stream.readUInt8(ambience.skyAbyssColor.b))
			wrap(stream.readUInt8(ambience.skyAbyssColor.a))
		} else if (core_memcmp(fieldId, "slc", 3) == 0) {
			wrap(stream.readUInt8(ambience.skyLightColor.r))
			wrap(stream.readUInt8(ambience.skyLightColor.g))
			wrap(stream.readUInt8(ambience.skyLightColor.b))
			wrap(stream.readUInt8(ambience.skyLightColor.a))
		} else if (core_memcmp(fieldId, "sli", 3) == 0) {
			wrap(stream.readFloat(ambience.skyLightIntensity));
		} else if (core_memcmp(fieldId, "foc", 3) == 0) {
			wrap(stream.readUInt8(ambience.fogColor.r))
			wrap(stream.readUInt8(ambience.fogColor.g))
			wrap(stream.readUInt8(ambience.fogColor.b))
			wrap(stream.readUInt8(ambience.fogColor.a))
		} else if (core_memcmp(fieldId, "fon", 3) == 0) {
			wrap(stream.readFloat(ambience.fogNear));
		} else if (core_memcmp(fieldId, "fof", 3) == 0) {
			wrap(stream.readFloat(ambience.fogFar));
		} else if (core_memcmp(fieldId, "foa", 3) == 0) {
			wrap(stream.readFloat(ambience.fogAbsorbtion));
		} else if (core_memcmp(fieldId, "suc", 3) == 0) {
			wrap(stream.readUInt8(ambience.sunColor.r))
			wrap(stream.readUInt8(ambience.sunColor.g))
			wrap(stream.readUInt8(ambience.sunColor.b))
			wrap(stream.readUInt8(ambience.sunColor.a))
		} else if (core_memcmp(fieldId, "sui", 3) == 0) {
			wrap(stream.readFloat(ambience.sunIntensity));
		} else if (core_memcmp(fieldId, "sur", 3) == 0) {
			wrap(stream.readFloat(ambience.sunRotation[0]));
			wrap(stream.readFloat(ambience.sunRotation[1]));
		} else if (core_memcmp(fieldId, "asl", 3) == 0) {
			wrap(stream.readFloat(ambience.ambientSkyLightFactor));
		} else if (core_memcmp(fieldId, "adl", 3) == 0) {
			wrap(stream.readFloat(ambience.ambientDirLightFactor));
		} else if (core_memcmp(fieldId, "txt", 3) == 0) {
			wrapBool(stream.readPascalStringUInt8(ambience.txt))
			Log::debug("ambience: txt: %s", ambience.txt.c_str());
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
		Log::debug("block key: %s", key.c_str());
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

int CubzhB64Format::load3zh(io::FilesystemArchive &archive, const core::String &filename,
							scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							const LoadContext &ctx) {
	io::SeekableReadStreamPtr stream = archive.readStream(filename);
	if (!stream) {
		Log::error("Failed to open file: %s", filename.c_str());
		return InvalidNodeId;
	}
	CubzhFormat format;
	scenegraph::SceneGraph modelScene;
	if (!format.load(filename, *stream.get(), modelScene, ctx)) {
		Log::error("Failed to load 3zh file: %s", filename.c_str());
		return InvalidNodeId;
	}
	// TODO: load all of them into a group node - this group node is the node we apply all properties to
	scenegraph::SceneGraph::MergedVolumePalette merged = modelScene.merge();
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setPalette(merged.second);
	node.setVolume(merged.first, true);
	return sceneGraph.emplace(core::move(node));
}

bool CubzhB64Format::readObjects(const core::String &filename, io::ReadStream &stream,
								 scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								 const LoadContext &ctx, int version) {
	if (version == 3) {
		uint32_t chunkLen;
		wrap(stream.readUInt32(chunkLen))
	} else {
		uint16_t chunkLen;
		wrap(stream.readUInt16(chunkLen))
	}
	uint16_t numObjects;
	wrap(stream.readUInt16(numObjects))
	Log::trace("numObjects: %i", numObjects);
	uint16_t instanceCount = 0;

	const core::String &path = core::string::extractPath(filename);

	// e.g. the hubmap.b64 is in bundle/misc, the 3zh files in bundle/cache
	io::FilesystemArchive archive(false);
	archive.add(core::string::path(path, "..", "cache"), "*.3zh", 1);
	archive.add(path, "*.3zh", 1);
	archive.add(core::string::path(path, "cache"), "*.3zh", 1);

	while (instanceCount < numObjects) {
		core::String fullname3zh;
		wrapBool(stream.readPascalStringUInt16LE(fullname3zh))

		uint16_t numInstances;
		wrap(stream.readUInt16(numInstances))
		Log::trace("numInstances: %i, instanceCount: %i, numObjects: %i", numInstances, instanceCount, numObjects);

		// load the 3zh file
		fullname3zh.replaceAllChars('.', '/'); // replace the lua dir separator
		fullname3zh.append(".3zh");

		const int modelNodeId = load3zh(archive, fullname3zh, sceneGraph, palette, ctx);
		if (modelNodeId == InvalidNodeId) {
			return false;
		}

		for (uint16_t j = 0; j < numInstances; ++j) {
			uint8_t numFields;
			wrap(stream.readUInt8(numFields))
			core::String uuid;
			core::String name;
			glm::vec3 pos{0.0f};
			glm::vec3 rot{0.0f};
			glm::vec3 scale{1.0f};
			uint8_t physicMode = 0;
			Log::trace("numFields: %i", numFields);
			for (uint8_t k = 0; k < numFields; ++k) {
				uint8_t fieldId[2];
				if (stream.read(fieldId, sizeof(fieldId)) != sizeof(fieldId)) {
					Log::error("Failed to read object field Id");
					return false;
				}
				Log::trace("%c%c", fieldId[0], fieldId[1]);
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
			scenegraph::SceneGraphNode *node;
			if (instanceCount > 1) {
				// TODO: don't load as reference - we would miss the rotations on merging the 3zh into a single node
				scenegraph::SceneGraphNode refNode(scenegraph::SceneGraphNodeType::ModelReference);
				core_assert_always(refNode.setReference(modelNodeId));
				const int refNodeId = sceneGraph.emplace(core::move(refNode), 0);
				if (refNodeId == InvalidNodeId) {
					Log::error("Failed to create reference node for model %i", modelNodeId);
					return false;
				}
				node = &sceneGraph.node(refNodeId);
			} else {
				node = &sceneGraph.node(modelNodeId);
			}
			node->setProperty("Physic mode", core::string::toString((int)physicMode));
			if (!uuid.empty()) {
				node->setProperty("uuid", uuid);
			}
			if (!name.empty()) {
				node->setName(name);
			}

			scenegraph::SceneGraphTransform transform;
			transform.setWorldTranslation(pos);
			transform.setWorldOrientation(glm::quat(rot));
			transform.setWorldScale(scale);
			scenegraph::KeyFrameIndex keyFrameIdx = 0;

			node->setTransform(keyFrameIdx, transform);
		}
	}
	return true;
}

void CubzhB64Format::setAmbienceProperties(scenegraph::SceneGraph &sceneGraph, const Ambience &ambience) const {
	auto &root = sceneGraph.node(sceneGraph.root().id());
	root.setProperty("sunColor", ambience.sunColor);
	root.setProperty("skyHorizonColor", ambience.skyHorizonColor);
	root.setProperty("skyAbyssColor", ambience.skyAbyssColor);
	root.setProperty("skyLightColor", ambience.skyLightColor);
	root.setProperty("skyLightIntensity", ambience.skyLightIntensity);

	root.setProperty("fogColor", ambience.fogColor);
	root.setProperty("fogNear", ambience.fogNear);
	root.setProperty("fogFar", ambience.fogFar);
	root.setProperty("fogAbsorbtion", ambience.fogAbsorbtion);

	root.setProperty("sunIntensity", ambience.sunIntensity);
	root.setProperty("sunRotation", core::string::format("%f:%f", ambience.sunRotation[0], ambience.sunRotation[1]));

	root.setProperty("ambientSkyLightFactor", ambience.ambientSkyLightFactor);
	root.setProperty("ambientDirLightFactor", ambience.ambientDirLightFactor);

	root.setProperty("txt", ambience.txt);

}

bool CubzhB64Format::loadVersion1(const core::String &filename, io::ReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	// TODO: not supported - base64 lua tables
	Ambience ambience;
	uint8_t chunkId;
	wrap(stream.readUInt8(chunkId))
	wrapBool(readChunkMap(stream, sceneGraph, palette, ctx))
	wrap(stream.readUInt8(chunkId))
	wrapBool(readAmbience(stream, sceneGraph, palette, ctx, ambience))
	wrap(stream.readUInt8(chunkId))
	wrapBool(readBlocks(stream, sceneGraph, palette, ctx))
	setAmbienceProperties(sceneGraph, ambience);
	return false;
}

bool CubzhB64Format::loadVersion2(const core::String &filename, io::ReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	Ambience ambience;
	while (!stream.eos()) {
		uint8_t chunkId;
		wrap(stream.readUInt8(chunkId))
		switch (chunkId) {
		case 0:
			wrapBool(readChunkMap(stream, sceneGraph, palette, ctx))
			break;
		case 1:
			wrapBool(readAmbience(stream, sceneGraph, palette, ctx, ambience))
			break;
		case 2:
			wrapBool(readObjects(filename, stream, sceneGraph, palette, ctx, 2))
			break;
		case 3:
			wrapBool(readBlocks(stream, sceneGraph, palette, ctx))
			break;
		default:
			Log::error("Unknown chunk id: %i", (int)chunkId);
			return false;
		}
	}
	setAmbienceProperties(sceneGraph, ambience);
	return true;
}

bool CubzhB64Format::loadVersion3(const core::String &filename, io::ReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	Ambience ambience;
	while (!stream.eos()) {
		uint8_t chunkId;
		wrap(stream.readUInt8(chunkId))
		Log::debug("chunk id: %u", chunkId);
		switch (chunkId) {
		case 0:
			wrapBool(readChunkMap(stream, sceneGraph, palette, ctx))
			break;
		case 1:
			wrapBool(readAmbience(stream, sceneGraph, palette, ctx, ambience))
			break;
		case 2:
			wrapBool(readObjects(filename, stream, sceneGraph, palette, ctx, 3))
			break;
		case 3:
			wrapBool(readBlocks(stream, sceneGraph, palette, ctx))
			break;
		default:
			Log::error("Unknown chunk id: %i", (int)chunkId);
			return false;
		}
	}
	setAmbienceProperties(sceneGraph, ambience);
	return true;
}

bool CubzhB64Format::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
									scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
									const LoadContext &ctx) {
	io::Base64ReadStream base64Stream(stream);
	uint8_t version;
	wrap(base64Stream.readUInt8(version))
	if (version == 1) {
		return loadVersion1(filename, base64Stream, sceneGraph, palette, ctx);
	} else if (version == 2) {
		return loadVersion2(filename, base64Stream, sceneGraph, palette, ctx);
	} else if (version == 3) {
		return loadVersion3(filename, base64Stream, sceneGraph, palette, ctx);
	}

	Log::error("Unsupported version found: %i", (int)version);
	return false;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
