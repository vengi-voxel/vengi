/**
 * @file
 */

#include "CubzhB64Format.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "io/Base64ReadStream.h"
#include "io/StreamUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/cubzh/CubzhFormat.h"
#include <stdint.h>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
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

bool CubzhB64Format::loadObject(const io::ArchivePtr &archive, const core::String &luaName,
								scenegraph::SceneGraph &modelScene, const LoadContext &ctx) {
	core::String fullPath = luaName;

	// load the 3zh file
	fullPath.replaceAllChars('.', '/'); // replace the lua dir separator
	fullPath.append(".3zh");

	if (!archive->exists(fullPath)) {
		const core::String &path = core::string::extractDir(fullPath);
		const core::String &file = core::string::extractFilenameWithExtension(fullPath);
		if (archive->exists(core::string::path(path, "..", "cache", file))) {
			fullPath = core::string::path(path, "..", "cache", file);
		} else if (archive->exists(core::string::path(path, "cache", file))) {
			fullPath = core::string::path(path, "cache", file);
		} else if (archive->exists(core::string::extractFilenameWithExtension(file))) {
			fullPath = core::string::extractFilenameWithExtension(file);
		} else {
			fullPath = luaName;

			// load the 3zh file
			fullPath.replaceAllChars('.', '-'); // replace the lua dir separator
			fullPath.append(".3zh");
			if (!archive->exists(fullPath)) {
				Log::error("3zh file not found: %s", fullPath.c_str());
				return false;
			}
		}
	}

	CubzhFormat format;
	if (!format.load(fullPath, archive, modelScene, ctx)) {
		Log::error("Failed to load 3zh file: %s", fullPath.c_str());
		return false;
	}
	return true;
}

bool CubzhB64Format::readChunkMap(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	double scale;
	wrap(stream.readDouble(scale)) // default is 5
	Log::debug("map scale: %f", scale);
	core::String name;
	wrapBool(stream.readPascalStringUInt32LE(name))
	Log::debug("map name: %s", name.c_str());

	scenegraph::SceneGraph modelScene;
	if (loadObject(archive, name, modelScene, ctx)) {
		scenegraph::copySceneGraph(sceneGraph, modelScene);
	} else {
		Log::warn("Failed to load 3zh file: %s", name.c_str());
	}
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
			wrapBool(io::readColor(stream, ambience.skyColor))
		} else if (core_memcmp(fieldId, "shc", 3) == 0) {
			wrapBool(io::readColor(stream, ambience.skyHorizonColor))
		} else if (core_memcmp(fieldId, "sac", 3) == 0) {
			wrapBool(io::readColor(stream, ambience.skyAbyssColor))
		} else if (core_memcmp(fieldId, "slc", 3) == 0) {
			wrapBool(io::readColor(stream, ambience.skyLightColor))
		} else if (core_memcmp(fieldId, "sli", 3) == 0) {
			wrap(stream.readFloat(ambience.skyLightIntensity));
		} else if (core_memcmp(fieldId, "foc", 3) == 0) {
			wrapBool(io::readColor(stream, ambience.fogColor))
		} else if (core_memcmp(fieldId, "fon", 3) == 0) {
			wrap(stream.readFloat(ambience.fogNear));
		} else if (core_memcmp(fieldId, "fof", 3) == 0) {
			wrap(stream.readFloat(ambience.fogFar));
		} else if (core_memcmp(fieldId, "foa", 3) == 0) {
			wrap(stream.readFloat(ambience.fogAbsorbtion));
		} else if (core_memcmp(fieldId, "suc", 3) == 0) {
			wrapBool(io::readColor(stream, ambience.sunColor))
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

	voxel::SparseVolume v;
	for (int i = 0; i < (int)numBlocks; ++i) {
		core::String key;
		wrapBool(stream.readPascalStringUInt16LE(key))
		Log::debug("block key: %s", key.c_str());
		uint8_t blockAction;
		wrap(stream.readUInt8(blockAction))
		if (blockAction == 1) {
			color::RGBA color;
			wrapBool(io::readColor(stream, color))
			// the index is an encoded position using: X + Y * 1000 + Z * 1000000
			const int x = i % 1000;
			const int y = i / 1000 % 1000;
			const int z = i / 1000000;
			v.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, color));
			Log::debug("set voxel to %i:%i:%i with color %s", x, y, z, color::toHex(color).c_str());
		}
	}

	const voxel::Region &sparseRegion = v.calculateRegion();
	if (!sparseRegion.isValid()) {
		Log::debug("Empty block");
		return true;
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName("Blocks");
	node.setVolume(new voxel::RawVolume(sparseRegion), true);
	v.copyTo(*node.volume());
	const int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to create blocks node");
		return false;
	}

	return true;
}

#define CHECK_ID(field, id) core_memcmp((field), (id), 2) == 0

bool CubzhB64Format::readObjects(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
								 scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								 const LoadContext &ctx, int version) {
	if (version == 3 || version == 2) {
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

	while (instanceCount < numObjects) {
		core::String luaName;
		wrapBool(stream.readPascalStringUInt16LE(luaName))

		uint16_t numInstances;
		wrap(stream.readUInt16(numInstances))
		Log::trace("numInstances: %i, instanceCount: %i, numObjects: %i", numInstances, instanceCount, numObjects);

		scenegraph::SceneGraph modelScene;
		if (!loadObject(archive, luaName, modelScene, ctx)) {
			Log::warn("Failed to load 3zh file: %s", luaName.c_str());
		}

		core::Buffer<int> modelNodeIds;
		for (uint16_t j = 0; j < numInstances; ++j) {
			uint8_t numFields;
			wrap(stream.readUInt8(numFields))
			core::String uuid;
			core::String name;
			glm::vec3 pos{0.0f};
			glm::vec3 rot{0.0f};
			glm::vec3 scale{1.0f}; // TODO: VOXELFORMAT: check this - might also be 0.5
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
					wrapBool(io::readVec3(stream, pos))
				} else if (CHECK_ID(fieldId, "ro")) {
					wrapBool(io::readVec3(stream, rot))
				} else if (CHECK_ID(fieldId, "sc")) {
					wrapBool(io::readVec3(stream, scale))
				} else if (CHECK_ID(fieldId, "na")) {
					wrapBool(stream.readPascalStringUInt8(name))
				} else if (CHECK_ID(fieldId, "de")) {
					core_assert(version == 2);
					core::String base64;
					// itemDetailsCell table
					wrapBool(stream.readPascalStringUInt16LE(base64))
					// TODO: VOXELFORMAT:
				} else if (CHECK_ID(fieldId, "pm")) {
					// Disabled = 0
					// Trigger = 1
					// TriggerPerBlock = 2
					// Static = 3
					// StaticPerBlock = 4 (default)
					// Dynamic = 5
					// TODO: VOXELFORMAT:
					wrap(stream.readUInt8(physicMode))
					wrap(stream.readUInt8(physicMode))
					wrap(stream.readUInt8(physicMode))
					wrap(stream.readUInt8(physicMode))
				} else if (CHECK_ID(fieldId, "cg")) {
					// CollisionGroups - read and skip for now
					uint8_t cg1, cg2, cg3, cg4;
					wrap(stream.readUInt8(cg1))
					wrap(stream.readUInt8(cg2))
					wrap(stream.readUInt8(cg3))
					wrap(stream.readUInt8(cg4))
					Log::debug("CollisionGroups: %u %u %u %u", cg1, cg2, cg3, cg4);
				} else if (CHECK_ID(fieldId, "cw")) {
					// CollidesWithGroups - read and skip for now
					uint8_t cw1, cw2, cw3, cw4;
					wrap(stream.readUInt8(cw1))
					wrap(stream.readUInt8(cw2))
					wrap(stream.readUInt8(cw3))
					wrap(stream.readUInt8(cw4))
					Log::debug("CollidesWithGroups: %u %u %u %u", cw1, cw2, cw3, cw4);
				} else {
					Log::error("Unknown field id: %c%c", fieldId[0], fieldId[1]);
					return false;
				}
			}

			// create a group node to apply the transforms to - this is needed to keep the original transforms of the
			// imported 3zh nodes
			scenegraph::SceneGraphNode instanceGroupNode(scenegraph::SceneGraphNodeType::Group, core::UUID(uuid));
			instanceGroupNode.setProperty("Physic mode", core::string::toString((int)physicMode));
			if (!name.empty()) {
				instanceGroupNode.setName(name);
			}

			scenegraph::SceneGraphTransform instanceGroupTransform;
			instanceGroupTransform.setWorldTranslation(pos);
			instanceGroupTransform.setWorldOrientation(glm::quat(rot));
			instanceGroupTransform.setWorldScale(scale);
			scenegraph::KeyFrameIndex keyFrameIdx = 0;
			instanceGroupNode.setTransform(keyFrameIdx, instanceGroupTransform);
			const int instanceGroupNodeId = sceneGraph.emplace(core::move(instanceGroupNode));
			if (instanceGroupNodeId == InvalidNodeId) {
				Log::error("Failed to create instance group node");
				return false;
			}

			++instanceCount;
			if (modelNodeIds.empty()) {
				if (!modelScene.empty()) {
					modelNodeIds = scenegraph::copySceneGraph(sceneGraph, modelScene, instanceGroupNodeId);
					if (modelNodeIds.empty()) {
						Log::error("Failed to copy scene graph from %s", luaName.c_str());
						// return false;
					}
					Log::debug("Added %i nodes from %s", (int)modelNodeIds.size(), luaName.c_str());
				}
			} else {
				for (int modelNodeId : modelNodeIds) {
					const int refNodeId =
						scenegraph::createNodeReference(sceneGraph, sceneGraph.node(modelNodeId), instanceGroupNodeId);
					if (refNodeId == InvalidNodeId) {
						Log::error("Failed to create reference node for model %i", modelNodeId);
						return false;
					}
				}
			}
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
	root.setProperty("sunRotation", core::String::format("%f:%f", ambience.sunRotation[0], ambience.sunRotation[1]));

	root.setProperty("ambientSkyLightFactor", ambience.ambientSkyLightFactor);
	root.setProperty("ambientDirLightFactor", ambience.ambientDirLightFactor);

	root.setProperty("txt", ambience.txt);
	root.setProperty(scenegraph::PropDescription, ambience.txt);
}

bool CubzhB64Format::loadVersion1(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	// TODO: VOXELFORMAT: not supported - base64 lua tables
	Ambience ambience;
	uint8_t chunkId;
	wrap(stream.readUInt8(chunkId))
	wrapBool(readChunkMap(filename, archive, stream, sceneGraph, palette, ctx))
	wrap(stream.readUInt8(chunkId))
	wrapBool(readAmbience(stream, sceneGraph, palette, ctx, ambience))
	wrap(stream.readUInt8(chunkId))
	wrapBool(readBlocks(stream, sceneGraph, palette, ctx))
	setAmbienceProperties(sceneGraph, ambience);
	return false;
}

bool CubzhB64Format::loadVersion2(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	Ambience ambience;
	while (!stream.eos()) {
		uint8_t chunkId;
		wrap(stream.readUInt8(chunkId))
		switch (chunkId) {
		case 0:
			wrapBool(readChunkMap(filename, archive, stream, sceneGraph, palette, ctx))
			break;
		case 1:
			wrapBool(readAmbience(stream, sceneGraph, palette, ctx, ambience))
			break;
		case 2:
			wrapBool(readObjects(filename, archive, stream, sceneGraph, palette, ctx, 2))
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

bool CubzhB64Format::loadVersion3(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	Ambience ambience;
	while (!stream.eos()) {
		uint8_t chunkId;
		wrap(stream.readUInt8(chunkId))
		Log::debug("chunk id: %u", chunkId);
		switch (chunkId) {
		case 0:
			wrapBool(readChunkMap(filename, archive, stream, sceneGraph, palette, ctx))
			break;
		case 1:
			wrapBool(readAmbience(stream, sceneGraph, palette, ctx, ambience))
			break;
		case 2:
			wrapBool(readObjects(filename, archive, stream, sceneGraph, palette, ctx, 3))
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

bool CubzhB64Format::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
									scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
									const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	io::Base64ReadStream base64Stream(*stream);
	uint8_t version;
	wrap(base64Stream.readUInt8(version))
	Log::debug("Found version %i", (int)version);
	if (version == 1) {
		if (!loadVersion1(filename, archive, base64Stream, sceneGraph, palette, ctx)) {
			return false;
		}
	} else if (version == 2) {
		if (!loadVersion2(filename, archive, base64Stream, sceneGraph, palette, ctx)) {
			return false;
		}
	} else if (version == 3) {
		if (!loadVersion3(filename, archive, base64Stream, sceneGraph, palette, ctx)) {
			return false;
		}
	} else {
		Log::error("Unsupported version found: %i", (int)version);
		return false;
	}
	Log::debug("%i bytes left in the stream", (int)stream->remaining());
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
