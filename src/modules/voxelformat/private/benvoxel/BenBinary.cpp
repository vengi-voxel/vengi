/**
 * @file
 */

#include "BenBinary.h"
#include "BenShared.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"

namespace voxelformat {

namespace benv {

static bool loadMetadataBinary(io::SeekableReadStream &stream, Metadata &metadata) {
	while (!stream.eos()) {
		ScopedChunkCheck chunk(stream);
		if (chunk.id == FourCC('P', 'R', 'O', 'P')) {
			uint16_t amount;
			if (stream.readUInt16(amount) != 0) {
				Log::error("Failed to read amount of properties");
				return false;
			}
			for (uint16_t i = 0; i < amount; ++i) {
				core::String name;
				if (!stream.readPascalStringUInt8(name)) {
					Log::error("Failed to read property name");
					return false;
				}
				core::String value;
				if (!stream.readPascalStringUInt32LE(value)) {
					Log::error("Failed to read property value");
					return false;
				}
				metadata.properties.emplace(name, core::move(value));
			}
		} else if (chunk.id == FourCC('P', 'T', '3', 'D')) {
			uint16_t amountPoints;
			if (stream.readUInt16(amountPoints) != 0) {
				Log::error("Failed to read amount of points");
				return false;
			}
			for (uint16_t i = 0; i < amountPoints; ++i) {
				core::String name;
				if (!stream.readPascalStringUInt8(name)) {
					Log::error("Failed to read point name");
					return false;
				}
				glm::ivec3 pointPos;
				if (stream.readInt32(pointPos.x) != 0 || stream.readInt32(pointPos.y) != 0 ||
					stream.readInt32(pointPos.z) != 0) {
					Log::error("Failed to read point position");
					return false;
				}
				metadata.points.emplace_back(name, pointPos);
			}
		} else if (chunk.id == FourCC('P', 'A', 'L', 'C')) {
			uint16_t amountPalettes;
			if (stream.readUInt16(amountPalettes) != 0) {
				Log::error("Failed to read amount of colors");
				return false;
			}
			for (uint16_t i = 0; i < amountPalettes; ++i) {
				core::String name;
				if (!stream.readPascalStringUInt8(name)) {
					Log::error("Failed to read palette name");
					return false;
				}
				palette::Palette palette;
				palette.setName(name);
				uint8_t entries;
				if (stream.readUInt8(entries) != 0) {
					Log::error("Failed to read amount of colors for palette %u", i);
					return false;
				}
				// 1 off so that it could fit the range of valid palette lengths (1-256) inside the valid range of byte
				// values (0-255)
				const int colors = (int)entries + 1;

				Log::debug("Palette %i/%i with name: '%s' and %i entries", (int)(i + 1), (int)amountPalettes,
						   name.c_str(), entries);
				for (int j = 0; j < colors; ++j) {
					core::RGBA color;
					if (stream.readUInt32(color.rgba) != 0) {
						Log::error("Failed to read color %u from %u for palette %u", j, entries, i);
						return false;
					}
					palette.tryAdd(color, false);
				}
				const bool hasDescriptions = stream.readBool();
				if (hasDescriptions) {
					for (int j = 0; j < colors; ++j) {
						core::String description;
						if (!stream.readPascalStringUInt32LE(description)) {
							Log::error("Failed to read description for palette %u", i);
							return false;
						}
						Log::debug("Description for palette %u entry: %u: %s", i, j, description.c_str());
					}
				}
				metadata.palettes.emplace(name, core::move(palette));
			}
		}
	}
	return true;
}

static bool loadModelBinary(scenegraph::SceneGraph &sceneGraph, const core::String &name, palette::Palette &palette,
							io::SeekableReadStream &stream, const Metadata &globalMetadata) {
	Metadata metadata;
	int nodeId = InvalidNodeId;
	while (!stream.eos()) {
		ScopedChunkCheck chunk(stream);
		if (chunk.id == FourCC('D', 'A', 'T', 'A')) {
			io::BufferedReadWriteStream dataStream(stream, chunk.length);
			if (!loadMetadataBinary(dataStream, metadata)) {
				Log::error("Failed to load metadata");
				return false;
			}
		} else if (chunk.id == FourCC('S', 'V', 'O', 'G')) {
			io::BufferedReadWriteStream dataStream(stream, chunk.length);
			uint16_t width, height, depth;
			if (dataStream.readUInt16(width) != 0 || dataStream.readUInt16(depth) != 0 || dataStream.readUInt16(height) != 0) {
				Log::error("Failed to read size of model");
				return false;
			}
			nodeId =
				createModelNode(sceneGraph, palette, name, width, height, depth, dataStream, globalMetadata, metadata);
			if (nodeId == InvalidNodeId) {
				return false;
			}
		}
	}
	for (const PointNode &pointNode : metadata.points) {
		if (!addPointNode(sceneGraph, pointNode.name, pointNode.pointPos)) {
			Log::error("Failed to add point node");
			return false;
		}
	}
	return true;
}

bool loadBinary(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, io::SeekableReadStream &stream) {
	Metadata globalMetadata;

	while (!stream.eos()) {
		ScopedChunkCheck chunk(stream);
		if (chunk.id == FourCC('D', 'A', 'T', 'A')) {
			Log::debug("Found metadata chunk");
			io::BufferedReadWriteStream dataStream(stream, chunk.length);
			if (!loadMetadataBinary(dataStream, globalMetadata)) {
				Log::error("Failed to load metadata");
				return false;
			}
			// empty name is default palette
			globalMetadata.palettes.get("", palette);
		} else {
			stream.seek(-8, SEEK_CUR);
		}

		uint16_t amount;
		if (stream.readUInt16(amount) != 0) {
			Log::error("Failed to read amount of models");
			return false;
		}

		Log::debug("%d entries", amount);

		for (uint16_t i = 0; i < amount; ++i) {
			core::String name;
			if (!stream.readPascalStringUInt8(name)) {
				Log::error("Failed to read model name");
				return false;
			}
			ScopedChunkCheck subChunk(stream);
			if (subChunk.id == FourCC('M', 'O', 'D', 'L')) {
				io::BufferedReadWriteStream modelStream(stream, subChunk.length);
				if (!loadModelBinary(sceneGraph, name, palette, modelStream, globalMetadata)) {
					Log::error("Failed to load model");
					return false;
				}
			} else {
				uint8_t buf[4];
				FourCCRev(buf, subChunk.id);
				Log::error("Unknown riff id with length %u: %c%c%c%c", subChunk.length, buf[0], buf[1], buf[2], buf[3]);
				stream.skipDelta(subChunk.length);
			}
		}
	}
	for (const PointNode &pointNode : globalMetadata.points) {
		if (!addPointNode(sceneGraph, pointNode.name, pointNode.pointPos)) {
			Log::error("Failed to add point node");
			return false;
		}
	}
	return true;
}

bool saveBinary(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream) {
	return false;
}

} // namespace benv

} // namespace voxelformat
