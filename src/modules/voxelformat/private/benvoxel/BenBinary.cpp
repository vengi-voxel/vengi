/**
 * @file
 */

#include "BenBinary.h"
#include "BenShared.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"

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
				if (stream.readInt32(pointPos.x) != 0 || stream.readInt32(pointPos.z) != 0 ||
					stream.readInt32(pointPos.y) != 0) {
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
				palette.setSize(colors);

				Log::debug("Palette %i/%i with name: '%s' and %i entries", (int)(i + 1), (int)amountPalettes,
						   name.c_str(), entries);
				for (int j = 0; j < colors; ++j) {
					color::RGBA color;
					if (stream.readUInt8(color.r) != 0) {
						Log::error("Failed to read color %u from %u for palette %u", j, entries, i);
						return false;
					}
					if (stream.readUInt8(color.g) != 0) {
						Log::error("Failed to read color %u from %u for palette %u", j, entries, i);
						return false;
					}
					if (stream.readUInt8(color.b) != 0) {
						Log::error("Failed to read color %u from %u for palette %u", j, entries, i);
						return false;
					}
					if (stream.readUInt8(color.a) != 0) {
						Log::error("Failed to read color %u from %u for palette %u", j, entries, i);
						return false;
					}
					palette.setColor(j, color);
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
			Log::debug("Found model metadata");
			io::BufferedReadWriteStream dataStream(stream, chunk.length);
			if (!loadMetadataBinary(dataStream, metadata)) {
				Log::error("Failed to load model metadata");
				return false;
			}
		} else if (chunk.id == FourCC('S', 'V', 'O', 'G')) {
			io::BufferedReadWriteStream dataStream(stream, chunk.length);
			uint16_t width, height, depth;
			if (dataStream.readUInt16(width) != 0 || dataStream.readUInt16(depth) != 0 ||
				dataStream.readUInt16(height) != 0) {
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
		// an empty name means that the volume is translated - this allows negative positions that are not directly
		// allowed in the svog chunk. We are not adding this point node to the scenegraph but instead we are shifting
		// the volume of the model node - when writing back we are also adding back the point node to the metadata for
		// any region that is not aligned at 0,0,0
		if (pointNode.name.empty()) {
			scenegraph::SceneGraphNode &modelNode = sceneGraph.node(nodeId);
			if (modelNode.isModelNode()) {
				voxel::RawVolume *volume = modelNode.volume();
				volume->region().shift(pointNode.pointPos);
				Log::debug("Shifted model '%s' by %f %f %f", name.c_str(), pointNode.pointPos.x, pointNode.pointPos.y,
						   pointNode.pointPos.z);
			}
		} else if (!addPointNode(sceneGraph, pointNode.name, pointNode.pointPos, nodeId)) {
			Log::error("Failed to add point node");
			return false;
		}
	}
	return true;
}

bool loadBinary(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, io::SeekableReadStream &stream) {
	uint32_t magic = 0;
	if (stream.readUInt32(magic) != 0) {
		Log::error("Failed to read magic");
		return false;
	}
	if (magic != FourCC('B', 'E', 'N', 'V')) {
		uint8_t buf[4];
		FourCCRev(buf, magic);
		Log::error("Invalid magic found - no binary benv file: %c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
		return false;
	}

	uint32_t totalLength;
	if (stream.readUInt32(totalLength) != 0) {
		Log::error("Failed to read total length");
		return false;
	}

	core::String version;
	if (!stream.readPascalStringUInt8(version)) {
		Log::error("Failed to read version");
		return false;
	}
	scenegraph::SceneGraphNode &root = sceneGraph.node(0);
	root.setProperty("version", version.c_str());

	io::ZipReadStream zipStream(stream, stream.remaining());
	io::BufferedReadWriteStream wrapper(zipStream);

	Metadata globalMetadata;

	while (!wrapper.eos()) {
		ScopedChunkCheck chunk(wrapper, false);
		if (chunk.id == FourCC('D', 'A', 'T', 'A')) {
			Log::debug("Found global metadata chunk");
			io::BufferedReadWriteStream dataStream(wrapper, chunk.length);
			if (!loadMetadataBinary(dataStream, globalMetadata)) {
				Log::error("Failed to load global metadata");
				return false;
			}
			// empty name is default palette
			globalMetadata.palettes.get("", palette);
		} else {
			wrapper.seek(-8, SEEK_CUR);
		}

		uint16_t amount;
		if (wrapper.readUInt16(amount) != 0) {
			Log::error("Failed to read amount of models");
			return false;
		}

		Log::debug("%d entries", amount);

		for (uint16_t i = 0; i < amount; ++i) {
			core::String name;
			if (!wrapper.readPascalStringUInt8(name)) {
				Log::error("Failed to read model name");
				return false;
			}
			ScopedChunkCheck subChunk(wrapper);
			if (subChunk.id == FourCC('M', 'O', 'D', 'L')) {
				io::BufferedReadWriteStream modelStream(wrapper, subChunk.length);
				if (!loadModelBinary(sceneGraph, name, palette, modelStream, globalMetadata)) {
					Log::error("Failed to load model");
					return false;
				}
			} else {
				uint8_t buf[4];
				FourCCRev(buf, subChunk.id);
				Log::error("Unknown riff id with length %u: %c%c%c%c", subChunk.length, buf[0], buf[1], buf[2], buf[3]);
				wrapper.skipDelta(subChunk.length);
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

static bool saveMetadataBinary(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
							   io::SeekableWriteStream &stream) {
	Chunk dataChunk(stream, FourCC('D', 'A', 'T', 'A'));
	Metadata metadata = createMetadata(sceneGraph, node);
	if (!metadata.points.empty()) {
		Chunk chunk(stream, FourCC('P', 'T', '3', 'D'));
		if (!stream.writeUInt16((uint16_t)metadata.points.size())) {
			Log::error("Failed to write amount of points");
			return false;
		}
		for (const PointNode &pointNode : metadata.points) {
			if (!stream.writePascalStringUInt8(pointNode.name)) {
				Log::error("Failed to write point name");
				return false;
			}
			if (!stream.writeInt32(pointNode.pointPos.x) || !stream.writeInt32(pointNode.pointPos.y) ||
				!stream.writeInt32(pointNode.pointPos.z)) {
				Log::error("Failed to write point position");
				return false;
			}
		}
	}
	if (!metadata.properties.empty()) {
		Chunk chunk(stream, FourCC('P', 'R', 'O', 'P'));
		if (!stream.writeUInt16((uint16_t)metadata.properties.size())) {
			Log::error("Failed to write amount of properties");
			return false;
		}
		for (const auto &entry : metadata.properties) {
			if (!stream.writePascalStringUInt8(entry->first)) {
				Log::error("Failed to write property name");
				return false;
			}
			if (!stream.writePascalStringUInt32LE(entry->second)) {
				Log::error("Failed to write property value");
				return false;
			}
		}
	}
	if (!metadata.palettes.empty()) {
		Chunk chunk(stream, FourCC('P', 'A', 'L', 'C'));
		if (!stream.writeUInt16((uint16_t)metadata.palettes.size())) {
			Log::error("Failed to write amount of palettes");
			return false;
		}
		for (const auto &entry : metadata.palettes) {
			const core::String &name = entry->first;
			const palette::Palette &palette = entry->second;
			if (!stream.writePascalStringUInt8(name)) {
				Log::error("Failed to write palette name");
				return false;
			}
			const int colors = palette.size();
			// 1 off so that it could fit the range of valid palette lengths (1-256) inside the valid range of byte
			// values (0-255)
			const int entries = colors - 1;
			if (!stream.writeUInt8((uint8_t)entries)) {
				Log::error("Failed to write amount of colors for palette %s", name.c_str());
				return false;
			}
			Log::debug("Palette '%s' with %i entries", name.c_str(), entries);
			for (int i = 0; i < colors; ++i) {
				const color::RGBA &color = palette.color(i);
				if (!stream.writeUInt8(color.a) || !stream.writeUInt8(color.b) || !stream.writeUInt8(color.g) ||
					!stream.writeUInt8(color.r)) {
					Log::error("Failed to write color %i for palette %s", i, name.c_str());
					return false;
				}
			}
			// TODO: VOXELFORMAT: save palette color names, too
			if (!stream.writeBool(false)) {
				Log::error("Failed to write description flag for palette %s", name.c_str());
				return false;
			}
		}
	}
	return true;
}

static bool saveModelBinary(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
							io::SeekableWriteStream &stream) {
	if (!stream.writePascalStringUInt8(node.name())) {
		Log::error("Failed to write model name");
		return false;
	}
	Chunk chunk(stream, FourCC('M', 'O', 'D', 'L'));
	{
		Chunk subChunk(stream, FourCC('D', 'A', 'T', 'A'));
		if (!saveMetadataBinary(sceneGraph, node, stream)) {
			Log::error("Failed to write metadata");
			return false;
		}
	}
	{
		Chunk subChunk(stream, FourCC('S', 'V', 'O', 'G'));
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		if (volume == nullptr) {
			Log::error("No volume found for model node %i", node.id());
			return false;
		}
		const glm::ivec3 &dim = volume->region().getDimensionsInVoxels();
		if (!stream.writeUInt16(dim.x) || !stream.writeUInt16(dim.z) || !stream.writeUInt16(dim.y)) {
			Log::error("Failed to write size of model");
			return false;
		}
		if (!saveModel(sceneGraph, node, stream, true)) {
			Log::error("Failed to save binary model for node %s", node.name().c_str());
			return false;
		}
	}
	return true;
}

bool saveBinary(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream) {
	uint32_t magic = FourCC('B', 'E', 'N', 'V');
	if (!stream.writeUInt32(magic)) {
		Log::error("Failed to write magic");
		return false;
	}

	int64_t totalLengthPos = stream.pos();
	if (!stream.writeUInt32(0)) {
		Log::error("Failed to read total length");
		return false;
	}

	core::String version = "0.0";
	if (!stream.writePascalStringUInt8(version)) {
		Log::error("Failed to read version");
		return false;
	}

	io::BufferedReadWriteStream wrapper;
	saveMetadataBinary(sceneGraph, sceneGraph.root(), wrapper);

	uint16_t amount = sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels);
	if (!wrapper.writeUInt16(amount)) {
		Log::error("Failed to write amount of models");
		return false;
	}
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->value;
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (!saveModelBinary(sceneGraph, node, wrapper)) {
			Log::error("Failed to write model");
			return false;
		}
	}

	if (wrapper.seek(0, SEEK_SET) == -1) {
		Log::error("Failed to seek to start of stream");
		return false;
	}
	const bool rawDeflate = true;
	io::ZipWriteStream zipStream(stream, 6, rawDeflate);
	if (!zipStream.writeStream(wrapper)) {
		Log::error("Failed to write zip stream");
		return false;
	}
	zipStream.flush();

	const int64_t totalLength = stream.pos() - totalLengthPos;
	if (stream.seek(totalLengthPos, SEEK_SET) == -1) {
		Log::error("Failed to seek to total length");
		return false;
	}
	if (!stream.writeUInt32((uint32_t)totalLength)) {
		Log::error("Failed to write total length");
		return false;
	}
	if (stream.seek(0, SEEK_END) == -1) {
		Log::error("Failed to seek to end");
		return false;
	}
	return true;
}

} // namespace benv

} // namespace voxelformat
