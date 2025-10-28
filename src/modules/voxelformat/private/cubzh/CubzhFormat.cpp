/**
 * @file
 */

#include "CubzhFormat.h"
#include "CubzhShared.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "io/StreamUtil.h"
#include "io/ZipReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
#include "json/JSON.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load 3zh file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load 3zh file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool CubzhFormat::Chunk::supportsCompression() const {
	return priv::supportsCompression(chunkId);
}

CubzhFormat::CubzhReadStream::CubzhReadStream(const Header &header, const Chunk &chunk,
											  io::SeekableReadStream &forward) {
	if (header.compressionType == 1 && chunk.compressed != 0) {
		Log::debug("load compressed chunk with id %d and size %d", chunk.chunkId, chunk.chunkSize);
		_stream = new io::ZipReadStream(forward, chunk.chunkSize);
		_forwarded = true;
		_size = chunk.uncompressedSize;
	} else {
		Log::debug("load uncompressed chunk with id %d and size %d", chunk.chunkId, chunk.chunkSize);
		_stream = &forward;
		_forwarded = false;
		_size = chunk.chunkSize;
	}
	_pos = 0;
}

CubzhFormat::CubzhReadStream::~CubzhReadStream() {
	if (_forwarded) {
		delete _stream;
	}
}

int CubzhFormat::CubzhReadStream::read(void *dataPtr, size_t dataSize) {
	if (dataSize > (size_t)remaining()) {
		Log::debug("requested to read %d bytes, but only %d are left", (int)dataSize, (int)remaining());
	}
	const int bytes = _stream->read(dataPtr, dataSize);
	if (bytes > 0) {
		_pos += bytes;
	}
	return bytes;
}

bool CubzhFormat::CubzhReadStream::eos() const {
	return pos() >= size();
}

int64_t CubzhFormat::CubzhReadStream::size() const {
	return _size;
}

int64_t CubzhFormat::CubzhReadStream::pos() const {
	return _pos;
}

int64_t CubzhFormat::CubzhReadStream::remaining() const {
	return size() - pos();
}

bool CubzhFormat::CubzhReadStream::empty() const {
	return size() == 0;
}

bool CubzhFormat::loadSkipChunk(const Header &header, const Chunk &chunk, io::ReadStream &stream) const {
	Log::debug("skip chunk %u with size %d", chunk.chunkId, (int)chunk.chunkSize);
	if (header.version == 6u && chunk.supportsCompression()) {
		Log::debug("skip additional header bytes for compressed chunk");
		// stream.skipDelta(5); // iscompressed byte and uncompressed size uint32
	}
	return stream.skipDelta(chunk.chunkSize) == 0;
}

bool CubzhFormat::loadSkipSubChunk(const Chunk &chunk, io::ReadStream &stream) const {
	Log::debug("skip subchunk %u", chunk.chunkId);
	return stream.skipDelta(chunk.chunkSize) == 0;
}

bool CubzhFormat::loadHeader(io::SeekableReadStream &stream, Header &header) const {
	uint8_t magic[6];
	if (stream.read(magic, sizeof(magic)) != (int)sizeof(magic)) {
		Log::error("Could not load 3zh magic: Not enough data in stream");
		return false;
	}

	if (!memcmp(magic, "CUBZH!", sizeof(magic))) {
		header.legacy = false;
		Log::debug("Found cubzh file");
	} else if (!memcmp(magic, "PARTIC", sizeof(magic))) {
		header.legacy = true;
		stream.skip(5); // UBES!
		Log::debug("Found particubes file");
	} else {
		Log::error("Could not load 3zh file: Invalid magic");
		return false;
	}

	wrap(stream.readUInt32(header.version))
	if (header.version != 5u && header.version != 6u) {
		Log::warn("Unsupported version %d", header.version);
	} else {
		Log::debug("Found version %d", header.version);
	}
	wrap(stream.readUInt8(header.compressionType));
	wrap(stream.readUInt32(header.totalSize))

	if (header.version == 5) {
		uint32_t uncompressedSize;
		wrap(stream.readUInt32(uncompressedSize))
	}

	Log::debug("CompressionType: %d", header.compressionType);
	Log::debug("Total size: %d", header.totalSize);

	return true;
}

bool CubzhFormat::loadPalettePCubes(io::ReadStream &stream, palette::Palette &palette) const {
	uint8_t colorCount;
	Log::debug("Found legacy palette");
	// rowCount
	// columnCount
	stream.skipDelta(2);
	uint16_t colorCount16;
	wrap(stream.readUInt16(colorCount16))
	colorCount = colorCount16;
	// default color
	// default background color
	stream.skipDelta(2);
	Log::debug("Palette with %d colors", colorCount);

	palette.setSize(colorCount);
	for (uint8_t i = 0; i < colorCount; ++i) {
		uint8_t r, g, b, a;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))
		wrap(stream.readUInt8(a))
		palette.setColor(i, core::RGBA(r, g, b, a));
	}
	for (uint8_t i = 0; i < colorCount; ++i) {
		const bool emissive = stream.readBool();
		if (emissive) {
			palette.setEmit(i, 1.0f);
		}
	}
	return true;
}

bool CubzhFormat::loadPalette5(io::ReadStream &stream, palette::Palette &palette, int version) const {
	uint8_t colorCount;
	Log::debug("Found v5 palette");
	if (version == 5) {
		uint8_t colorEncoding;
		wrap(stream.readUInt8(colorEncoding))
		if (colorEncoding != 1) {
			Log::error("Unsupported color encoding %d", colorEncoding);
			return false;
		}
	}
	uint8_t rowCount;
	wrap(stream.readUInt8(rowCount))
	uint8_t columnCount;
	wrap(stream.readUInt8(columnCount))
	uint16_t colorCount16;
	wrap(stream.readUInt16(colorCount16))
	colorCount = colorCount16;

	if (colorCount16 != ((uint16_t)rowCount * (uint16_t)columnCount)) {
		Log::error("Invalid color count %d", colorCount16);
		return 0;
	}

	if (version == 5) {
		uint8_t defaultColor;
		wrap(stream.readUInt8(defaultColor))
		uint8_t defaultBackgroundColor;
		wrap(stream.readUInt8(defaultBackgroundColor))
	}

	palette.setSize(colorCount);
	for (uint8_t i = 0; i < colorCount; ++i) {
		uint8_t r, g, b, a;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))
		wrap(stream.readUInt8(a))
		palette.setColor(i, core::RGBA(r, g, b, a));
	}
	// default color
	// default background color
	stream.skipDelta(2);
	Log::debug("Palette with %d colors", colorCount);

	return true;
}

bool CubzhFormat::loadPalette6(io::ReadStream &stream, palette::Palette &palette) const {
	uint8_t colorCount = 0;
	wrap(stream.readUInt8(colorCount))
	Log::debug("Palette with %d colors", colorCount);

	palette.setSize(colorCount);
	for (uint8_t i = 0; i < colorCount; ++i) {
		uint8_t r, g, b, a;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))
		wrap(stream.readUInt8(a))
		palette.setColor(i, core::RGBA(r, g, b, a));
	}
	for (uint8_t i = 0; i < colorCount; ++i) {
		const bool emissive = stream.readBool();
		if (emissive) {
			palette.setEmit(i, 1.0f);
		}
	}
	return true;
}

bool CubzhFormat::loadShape5(const core::String &filename, const Header &header, const Chunk &chunk,
							 io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
							 const palette::Palette &palette, const LoadContext &ctx) const {
	uint16_t width = 0, depth = 0, height = 0;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);

	int64_t startPos = stream.pos();
	while (stream.pos() < startPos + chunk.chunkSize - 5) {
		Chunk subChunk;
		wrapBool(loadSubChunkHeader(header, stream, subChunk))
		core::Buffer<uint8_t> volumeBuffer; // used in case the size chunk is late
		switch (subChunk.chunkId) {
		case priv::CHUNK_ID_SHAPE_SIZE_V5:
			wrap(stream.readUInt16(width))
			wrap(stream.readUInt16(height))
			wrap(stream.readUInt16(depth))
			Log::debug("Found size chunk: %i:%i:%i", width, height, depth);

			if (!volumeBuffer.empty()) {
				const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
				if (!region.isValid()) {
					Log::error("Invalid region: %i:%i:%i", width, height, depth);
					return false;
				}

				voxel::RawVolume *volume = new voxel::RawVolume(region);
				node.setVolume(volume, true);
				int i = 0;
				if (width * height * depth > (int)volumeBuffer.size()) {
					Log::error("invalid blocks chunk");
					return false;
				}
				voxel::RawVolume::Sampler sampler(volume);
				sampler.setPosition(width - 1, 0, 0);
				for (uint16_t x = 0; x < width; x++) {
					voxel::RawVolume::Sampler sampler2 = sampler;
					for (uint16_t y = 0; y < height; y++) {
						voxel::RawVolume::Sampler sampler3 = sampler2;
						for (uint16_t z = 0; z < depth; z++) {
							const uint8_t index = volumeBuffer[i++];
							if (index == emptyPaletteIndex()) {
								continue;
							}
							const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
							sampler3.setVoxel(voxel);
							sampler3.movePositiveZ();
						}
						sampler2.movePositiveY();
					}
					sampler.moveNegativeX();
				}
			}
			break;
		case priv::CHUNK_ID_SHAPE_BLOCKS_V5: {
			Log::debug("Shape with %u voxels found", subChunk.chunkSize);
			if (width == 0) {
				volumeBuffer.reserve(subChunk.chunkSize);
				for (uint32_t i = 0; i < subChunk.chunkSize; ++i) {
					uint8_t index;
					wrap(stream.readUInt8(index))
					volumeBuffer.push_back(index);
				}
				break;
			}
			uint32_t voxelCount = (uint32_t)width * (uint32_t)height * (uint32_t)depth;
			if (voxelCount * sizeof(uint8_t) != subChunk.chunkSize) {
				Log::error("Invalid size for blocks chunk: %i", subChunk.chunkSize);
				return false;
			}
			const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
			if (!region.isValid()) {
				Log::error("Invalid region: %i:%i:%i", width, height, depth);
				return false;
			}

			voxel::RawVolume *volume = new voxel::RawVolume(region);
			node.setVolume(volume, true);
			voxel::RawVolume::Sampler sampler(volume);
			sampler.setPosition(width - 1, 0, 0);
			for (uint16_t x = 0; x < width; x++) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (uint16_t y = 0; y < height; y++) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (uint16_t z = 0; z < depth; z++) {
						uint8_t index;
						wrap(stream.readUInt8(index))
						if (index == emptyPaletteIndex()) {
							sampler3.movePositiveZ();
							continue;
						}
						const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
						sampler3.setVoxel(voxel);
						sampler3.movePositiveZ();
					}
					sampler2.movePositiveY();
				}
				sampler.moveNegativeX();
			}
			break;
		}
		case priv::CHUNK_ID_SHAPE_POINT_V5: {
			core::String name;
			wrapBool(stream.readString(subChunk.chunkSize, name))
			float f3x, f3y, f3z;
			wrap(stream.readFloat(f3x))
			wrap(stream.readFloat(f3y))
			wrap(stream.readFloat(f3z))
			node.setProperty(name, core::String::format("%f:%f:%f", f3x, f3y, f3z));
			break;
		}
		default:
			wrapBool(loadSkipSubChunk(subChunk, stream))
			break;
		}
	}
	if (node.volume() == nullptr) {
		Log::error("No volume found in v5 file");
		return false;
	}
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool CubzhFormat::loadVersion5(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
							   const LoadContext &ctx) const {
	while (!stream.eos()) {
		Chunk chunk;
		wrapBool(loadChunkHeader(header, stream, chunk))
		ChunkChecker check(stream, chunk);
		switch (chunk.chunkId) {
		case priv::CHUNK_ID_PALETTE_V5:
			if (!loadPalette5(stream, palette, 5)) {
				return false;
			}
			break;
		case priv::CHUNK_ID_SHAPE_V5:
			if (!loadShape5(filename, header, chunk, stream, sceneGraph, palette, ctx)) {
				return false;
			}
			break;
		default:
			wrapBool(loadSkipChunk(header, chunk, stream))
			break;
		}
	}

	return true;
}

bool CubzhFormat::loadPCubes(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
							 scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
							 const LoadContext &ctx) const {
	while (!stream.eos()) {
		Chunk chunk;
		wrapBool(loadChunkHeader(header, stream, chunk))
		ChunkChecker check(stream, chunk);
		switch (chunk.chunkId) {
		case priv::CHUNK_ID_PALETTE_LEGACY_V6: {
			Log::debug("load palette");
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadPalettePCubes(zhs, palette))
			break;
		}
		case priv::CHUNK_ID_SHAPE_V6: {
			Log::debug("load shape");
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadShape6(filename, header, chunk, zhs, sceneGraph, palette, ctx))
			break;
		}
		default:
			Log::debug("Skip chunk %d", chunk.chunkId);
			wrapBool(loadSkipChunk(header, chunk, stream))
			break;
		}
	}
	return true;
}

bool CubzhFormat::loadChunkHeader(const Header &header, io::ReadStream &stream, Chunk &chunk) const {
	wrapBool(loadSubChunkHeader(header, stream, chunk))
	Log::debug("Mainchunk id %u with size %u", chunk.chunkId, chunk.chunkSize);
	if (header.version == 6u && chunk.supportsCompression()) {
		wrap(stream.readUInt8(chunk.compressed))
		wrap(stream.readUInt32(chunk.uncompressedSize))
		Log::debug("Compressed: %u", chunk.compressed);
		Log::debug("Uncompressed size: %u", chunk.uncompressedSize);
	}
	return true;
}

bool CubzhFormat::loadSubChunkHeader(const Header &header, io::ReadStream &stream, Chunk &chunk) const {
	wrap(stream.readUInt8(chunk.chunkId))
	if (header.version == 6 && chunk.chunkId == priv::CHUNK_ID_SHAPE_NAME_V6) {
		uint8_t chunkSize;
		wrap(stream.readUInt8(chunkSize))
		chunk.chunkSize = chunkSize;
	} else {
		wrap(stream.readUInt32(chunk.chunkSize))
	}
	Log::debug("Chunk id %u with size %u", chunk.chunkId, chunk.chunkSize);
	return true;
}

bool CubzhFormat::loadShape6(const core::String &filename, const Header &header, const Chunk &chunk,
							 CubzhReadStream &stream, scenegraph::SceneGraph &sceneGraph,
							 const palette::Palette &palette, const LoadContext &ctx) const {
	uint16_t width = 0, depth = 0, height = 0;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(core::string::extractFilename(filename));
	uint16_t shapeId = 1;
	uint16_t parentShapeId = 0;
	glm::vec3 pivot{0.5f}; // default is center of shape
	glm::vec3 pos{0};
	glm::vec3 eulerAngles{0};
	glm::vec3 scale{1};
	palette::Palette nodePalette = palette;
	bool hasPivot = false;
	bool sizeChunkFound = false;
	bool paletteFound = false;
	bool nameFound = false;
	while (!stream.eos()) {
		if (stream.remaining() == 4 && nameFound) {
			// there is a bug in the calculation of the uncompressed size in cubzh that writes a few bytes
			// too much for the size of the name chunk
			break;
		}
		Log::debug("Remaining sub stream data: %d", (int)stream.remaining());
		Chunk subChunk;
		wrapBool(loadSubChunkHeader(header, stream, subChunk))
		core::Buffer<uint8_t> volumeBuffer; // used in case the size chunk is late
		switch (subChunk.chunkId) {
		case priv::CHUNK_ID_SHAPE_ID_V6:
			wrap(stream.readUInt16(shapeId))
			Log::debug("Load shape id %u", shapeId);
			node.setProperty("shapeId", core::String::format("%d", shapeId));
			break;
		case priv::CHUNK_ID_SHAPE_PARENT_ID_V6:
			wrap(stream.readUInt16(parentShapeId))
			Log::debug("Load parent id %u", parentShapeId);
			break;
		case priv::CHUNK_ID_SHAPE_TRANSFORM_V6: {
			Log::debug("Load local transform");
			wrapBool(io::readVec3(stream, pos))
			wrapBool(io::readVec3(stream, eulerAngles))
			wrapBool(io::readVec3(stream, scale))
			break;
		}
		case priv::CHUNK_ID_SHAPE_PIVOT_V6: {
			Log::debug("Load pivot");
			wrapBool(io::readVec3(stream, pivot))
			hasPivot = true;
			Log::debug("pivot: %f:%f:%f", pivot.x, pivot.y, pivot.z);
			break;
		}
		case priv::CHUNK_ID_SHAPE_PALETTE_V6: {
			wrapBool(loadPalette6(stream, nodePalette))
			paletteFound = true;
			break;
		}
		case priv::CHUNK_ID_OBJECT_COLLISION_BOX_V6: {
			Log::debug("Load collision box");
			glm::vec3 mins;
			wrapBool(io::readVec3(stream, mins))
			glm::vec3 maxs;
			wrapBool(io::readVec3(stream, maxs))
			break;
		}
		case priv::CHUNK_ID_OBJECT_IS_HIDDEN_V6: {
			Log::debug("Load hidden state");
			node.setVisible(!stream.readBool());
			break;
		}
		case priv::CHUNK_ID_SHAPE_NAME_V6: {
			core::String name;
			wrapBool(stream.readString(subChunk.chunkSize, name));
			if (!name.empty()) {
				node.setName(name);
			}
			nameFound = true;
			Log::debug("Load node name: %s", name.c_str());
			break;
		}
		case priv::CHUNK_ID_SHAPE_SIZE_V6:
			Log::debug("Load shape size");
			wrap(stream.readUInt16(width))
			wrap(stream.readUInt16(height))
			wrap(stream.readUInt16(depth))
			Log::debug("Found size chunk: %i:%i:%i", width, height, depth);
			sizeChunkFound = width > 0 && height > 0 && depth > 0;
			if (!sizeChunkFound) {
				Log::warn("Invalid size chunk: %i:%i:%i", width, height, depth);
			}

			if (!volumeBuffer.empty()) {
				const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
				if (!region.isValid()) {
					Log::error("Invalid region: %i:%i:%i", width, height, depth);
					return false;
				}

				voxel::RawVolume *volume = new voxel::RawVolume(region);
				node.setVolume(volume, true);
				int i = 0;
				if (width * height * depth > (int)volumeBuffer.size()) {
					Log::error("invalid blocks chunk");
					return false;
				}
				voxel::RawVolume::Sampler sampler(volume);
				sampler.setPosition(width - 1, 0, 0);
				for (uint16_t x = 0; x < width; x++) {
					voxel::RawVolume::Sampler sampler2 = sampler;
					for (uint16_t y = 0; y < height; y++) {
						voxel::RawVolume::Sampler sampler3 = sampler2;
						for (uint16_t z = 0; z < depth; z++) {
							const uint8_t index = volumeBuffer[i++];
							if (index == emptyPaletteIndex()) {
								sampler3.movePositiveZ();
								continue;
							}
							const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
							sampler3.setVoxel(voxel);
							sampler3.movePositiveZ();
						}
						sampler2.movePositiveY();
					}
					sampler.moveNegativeX();
				}
			}
			break;
		case priv::CHUNK_ID_SHAPE_BLOCKS_V6: {
			Log::debug("Shape with %u voxels found", subChunk.chunkSize);
			if (width == 0) {
				volumeBuffer.reserve(subChunk.chunkSize);
				for (uint32_t i = 0; i < subChunk.chunkSize; ++i) {
					uint8_t index;
					wrap(stream.readUInt8(index))
					volumeBuffer.push_back(index);
				}
				break;
			}
			uint32_t voxelCount = (uint32_t)width * (uint32_t)height * (uint32_t)depth;
			if (voxelCount * sizeof(uint8_t) != subChunk.chunkSize) {
				Log::error("Invalid size for blocks chunk: %i", subChunk.chunkSize);
				return false;
			}
			const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
			if (!region.isValid()) {
				Log::error("Invalid region: %i:%i:%i", width, height, depth);
				return false;
			}

			voxel::RawVolume *volume = new voxel::RawVolume(region);
			node.setVolume(volume, true);
			voxel::RawVolume::Sampler sampler(volume);
			sampler.setPosition(width - 1, 0, 0);
			for (uint16_t x = 0; x < width; x++) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (uint16_t y = 0; y < height; y++) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (uint16_t z = 0; z < depth; z++) {
						uint8_t index;
						wrap(stream.readUInt8(index))
						if (index == emptyPaletteIndex()) {
							sampler3.movePositiveZ();
							continue;
						}
						const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
						sampler3.setVoxel(voxel);
						sampler3.movePositiveZ();
					}
					sampler2.movePositiveY();
				}
				sampler.moveNegativeX();
			}
			break;
		}
		case priv::CHUNK_ID_SHAPE_POINT_V6: {
			Log::debug("Load shape point position");
			core::String name;
			wrapBool(stream.readPascalStringUInt8(name))
			glm::vec3 poiPos;
			wrapBool(io::readVec3(stream, poiPos))
			if (scenegraph::SceneGraphNode *existingNode = sceneGraph.findNodeByName(name)) {
				scenegraph::SceneGraphTransform &transform = existingNode->transform(0);
				transform.setLocalTranslation(poiPos);
			} else {
				scenegraph::SceneGraphNode pointNode(scenegraph::SceneGraphNodeType::Point);
				pointNode.setName(name);
				scenegraph::SceneGraphTransform transform;
				transform.setLocalTranslation(poiPos);
				pointNode.setTransform(0, transform);
				sceneGraph.emplace(core::move(pointNode), node.id());
			}
			break;
		}
		case priv::CHUNK_ID_SHAPE_POINT_ROTATION_V6: {
			Log::debug("Load shape point rotation");
			core::String name;
			wrapBool(stream.readPascalStringUInt8(name))
			glm::vec3 poiAngles;
			wrapBool(io::readVec3(stream, poiAngles))
			if (scenegraph::SceneGraphNode *existingNode = sceneGraph.findNodeByName(name)) {
				scenegraph::SceneGraphTransform &transform = existingNode->transform(0);
				transform.setLocalOrientation(glm::quat(poiAngles));
			} else {
				scenegraph::SceneGraphNode pointNode(scenegraph::SceneGraphNodeType::Point);
				pointNode.setName(name);
				scenegraph::SceneGraphTransform transform;
				transform.setLocalTranslation(poiAngles);
				pointNode.setTransform(0, transform);
				sceneGraph.emplace(core::move(pointNode), node.id());
			}
			break;
		}
		case priv::CHUNK_ID_SHAPE_BAKED_LIGHTING_V6:
		default:
			Log::debug("Ignore subchunk %u", subChunk.chunkId);
			wrapBool(loadSkipSubChunk(subChunk, stream))
			break;
		}
	}

	if (node.volume() == nullptr) {
		if (sizeChunkFound) {
			node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		} else {
			Log::error("No volume found");
			return false;
		}
	}
	scenegraph::SceneGraphTransform transform;
	transform.setLocalTranslation(pos);
	transform.setLocalOrientation(glm::quat(eulerAngles));
	transform.setLocalScale(scale);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	if (hasPivot && sizeChunkFound) {
		core_assert(width != 0);
		core_assert(height != 0);
		core_assert(depth != 0);
		pivot.x /= (float)width;
		pivot.y /= (float)height;
		pivot.z /= (float)depth;
	}
	node.setPivot(pivot);
	node.setPalette(nodePalette);
	int parent = 0;
	if (parentShapeId != 0) {
		if (scenegraph::SceneGraphNode *parentNode =
				sceneGraph.findNodeByPropertyValue("shapeId", core::String::format("%d", parentShapeId))) {
			parent = parentNode->id();
			if (!paletteFound) {
				node.setPalette(parentNode->palette());
			}
		} else {
			Log::warn("Could not find node with parent shape id %d", parentShapeId);
		}
	}
	return sceneGraph.emplace(core::move(node), parent) != InvalidNodeId;
}

bool CubzhFormat::loadVersion6(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
							   const LoadContext &ctx) const {
	while (!stream.eos()) {
		Log::debug("Remaining stream data: %d", (int)stream.remaining());
		Chunk chunk;
		wrapBool(loadChunkHeader(header, stream, chunk))
		ChunkChecker check(stream, chunk);
		if (chunk.chunkId < priv::CHUNK_ID_MIN || chunk.chunkId > priv::CHUNK_ID_MAX_V6) {
			Log::warn("Invalid chunk id found: %u", chunk.chunkId);
			break;
		}
		switch (chunk.chunkId) {
		case priv::CHUNK_ID_PALETTE_V6: {
			Log::debug("load v6 palette");
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadPalette6(zhs, palette))
			break;
		}
		case priv::CHUNK_ID_PALETTE_LEGACY_V6: {
			Log::debug("load legacy palette");
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadPalette5(zhs, palette, 6))
			break;
		}
		case priv::CHUNK_ID_SHAPE_V6: {
			Log::debug("load shape");
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadShape6(filename, header, chunk, zhs, sceneGraph, palette, ctx))
			break;
		}
		case priv::CHUNK_ID_CAMERA_V6: {
			Log::debug("ignore camera");
			wrapBool(loadSkipChunk(header, chunk, stream))
			break;
		}
		case priv::CHUNK_ID_GENERAL_RENDERING_OPTIONS_V6: {
			Log::debug("ignore rendering options");
			wrapBool(loadSkipChunk(header, chunk, stream))
			break;
		}
		case priv::CHUNK_ID_DIRECTIONAL_LIGHT_V6: {
			Log::debug("ignore directional light");
			wrapBool(loadSkipChunk(header, chunk, stream))
			break;
		}
		default:
			Log::debug("ignore chunk with id %i", chunk.chunkId);
			wrapBool(loadSkipChunk(header, chunk, stream))
			break;
		}
	}

	return true;
}

bool CubzhFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return 0;
	}
	Header header;
	wrapBool(loadHeader(*stream, header))
	Log::debug("Found version %d", header.version);
	if (header.legacy) {
		if (!loadPCubes(filename, header, *stream, sceneGraph, palette, ctx)) {
			return false;
		}
	} else if (header.version == 5) {
		if (!loadVersion5(filename, header, *stream, sceneGraph, palette, ctx)) {
			return false;
		}
	} else {
		if (!loadVersion6(filename, header, *stream, sceneGraph, palette, ctx)) {
			return false;
		}
	}
	return loadAnimations(filename, archive, sceneGraph, ctx);
}

size_t CubzhFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
								const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return 0;
	}
	Header header;
	wrapBool(loadHeader(*stream, header))
	while (!stream->eos()) {
		Chunk chunk;
		wrapBool(loadChunkHeader(header, *stream, chunk))
		ChunkChecker check(*stream, chunk);
		if (header.version == 5u && chunk.chunkId == priv::CHUNK_ID_PALETTE_V5) {
			wrapBool(loadPalettePCubes(*stream, palette))
			return palette.size();
		} else if (header.version == 6u && chunk.chunkId == priv::CHUNK_ID_PALETTE_V6) {
			CubzhReadStream zhs(header, chunk, *stream);
			wrapBool(loadPalette6(zhs, palette))
			return palette.size();
		} else if (header.version == 6u && chunk.chunkId == priv::CHUNK_ID_PALETTE_LEGACY_V6) {
			CubzhReadStream zhs(header, chunk, *stream);
			wrapBool(loadPalettePCubes(zhs, palette))
			return palette.size();
		} else {
			wrapBool(loadSkipChunk(header, chunk, *stream))
		}
	}
	return palette.size();
}

image::ImagePtr CubzhFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
											const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return image::ImagePtr();
	}
	Header header;
	if (!loadHeader(*stream, header)) {
		Log::error("Failed to read header");
		return image::ImagePtr();
	}
	while (!stream->eos()) {
		Chunk chunk;
		if (!loadChunkHeader(header, *stream, chunk)) {
			return image::ImagePtr();
		}
		if (chunk.chunkId == priv::CHUNK_ID_PREVIEW) {
			image::ImagePtr img = image::createEmptyImage(core::string::extractFilename(filename) + ".png");
			img->load(image::ImageType::PNG, *stream, chunk.chunkSize);
			return img;
		}
		if (!loadSkipChunk(header, chunk, *stream)) {
			Log::error("Failed to skip chunk %d with size %d", chunk.chunkId, chunk.chunkSize);
			break;
		}
	}
	return image::ImagePtr();
}

#undef wrap
#undef wrapBool

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Could not save 3zh file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

bool CubzhFormat::savePointNodes(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								 io::SeekableWriteStream &ws) const {
	for (auto childId : node.children()) {
		const scenegraph::SceneGraphNode &child = sceneGraph.node(childId);
		if (child.type() != scenegraph::SceneGraphNodeType::Point) {
			continue;
		}
		{
			WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_POINT_V6, ws);
			core::String name = child.name();
			glm::vec3 pos = child.transform(0).localTranslation();
			wrapBool(sub.writePascalStringUInt8(name))
			wrapBool(sub.writeFloat(pos.x))
			wrapBool(sub.writeFloat(pos.y))
			wrapBool(sub.writeFloat(pos.z))
		}
#if 0
		{
			core::String name = "TODO";
			glm::vec3 poiPos;
			wrapBool(ws.writeUInt8(priv::CHUNK_ID_SHAPE_POINT_ROTATION_V6))
			wrapBool(ws.writePascalStringUInt8(name))
			wrapBool(ws.writeFloat(poiPos.x))
			wrapBool(ws.writeFloat(poiPos.y))
			wrapBool(ws.writeFloat(poiPos.z))
		}
#endif
	}
	return true;
}

bool CubzhFormat::saveModelNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								io::SeekableWriteStream *stream) const {
	WriteChunkStream ws(priv::CHUNK_ID_SHAPE_V6, *stream);
	{
		WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_ID_V6, ws);
		wrapBool(sub.writeUInt16(node.id()))
	}
	if (node.parent() != sceneGraph.root().id()) {
		WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_PARENT_ID_V6, ws);
		wrapBool(sub.writeUInt16(node.parent()))
	}
	{
		WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_TRANSFORM_V6, ws);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
		const glm::vec3 pos = transform.localTranslation();
		const glm::vec3 eulerAngles = glm::eulerAngles(transform.localOrientation());
		const glm::vec3 scale = transform.localScale();
		wrapBool(sub.writeFloat(pos.x))
		wrapBool(sub.writeFloat(pos.y))
		wrapBool(sub.writeFloat(pos.z))
		wrapBool(sub.writeFloat(eulerAngles.x))
		wrapBool(sub.writeFloat(eulerAngles.y))
		wrapBool(sub.writeFloat(eulerAngles.z))
		wrapBool(sub.writeFloat(scale.x))
		wrapBool(sub.writeFloat(scale.y))
		wrapBool(sub.writeFloat(scale.z))
	}
	{
		WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_PIVOT_V6, ws);
		const glm::vec3 &pivot = node.worldPivot();
		wrapBool(sub.writeFloat(pivot.x))
		wrapBool(sub.writeFloat(pivot.y))
		wrapBool(sub.writeFloat(pivot.z))
	}
	if (node.palette().colorCount() > 0) {
		WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_PALETTE_V6, ws);
		const palette::Palette &palette = node.palette();
		const uint8_t colorCount = palette.colorCount();
		sub.writeUInt8(colorCount);
		for (uint8_t i = 0; i < colorCount; ++i) {
			const core::RGBA rgba = palette.color(i);
			wrapBool(sub.writeUInt8(rgba.r))
			wrapBool(sub.writeUInt8(rgba.g))
			wrapBool(sub.writeUInt8(rgba.b))
			wrapBool(sub.writeUInt8(rgba.a))
		}
		for (uint8_t i = 0; i < colorCount; ++i) {
			wrapBool(sub.writeBool(palette.hasEmit(i)))
		}
	}
	{
		WriteSubChunkStream sub(priv::CHUNK_ID_OBJECT_COLLISION_BOX_V6, ws);
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		const glm::ivec3 mins = region.getLowerCorner();
		const glm::ivec3 maxs = region.getUpperCorner() + 1;
		wrapBool(sub.writeFloat(mins.x))
		wrapBool(sub.writeFloat(mins.y))
		wrapBool(sub.writeFloat(mins.z))
		wrapBool(sub.writeFloat(maxs.x))
		wrapBool(sub.writeFloat(maxs.y))
		wrapBool(sub.writeFloat(maxs.z))
	}
	{
		WriteSubChunkStream sub(priv::CHUNK_ID_OBJECT_IS_HIDDEN_V6, ws);
		sub.writeBool(!node.visible());
	}
	{
		WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_SIZE_V6, ws);
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		const glm::ivec3 &dimensions = region.getDimensionsInVoxels();
		wrapBool(sub.writeUInt16(dimensions.x))
		wrapBool(sub.writeUInt16(dimensions.y))
		wrapBool(sub.writeUInt16(dimensions.z))
	}
	{
		WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_BLOCKS_V6, ws);
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		const voxel::Region &region = volume->region();
		const uint8_t emptyColorIndex = (uint8_t)emptyPaletteIndex();
		for (int x = region.getUpperX(); x >= region.getLowerX(); x--) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); y++) {
				for (int z = region.getLowerZ(); z <= region.getUpperZ(); z++) {
					const voxel::Voxel &voxel = volume->voxel(x, y, z);
					if (voxel::isAir(voxel.getMaterial())) {
						wrapBool(sub.writeUInt8(emptyColorIndex))
					} else {
						wrapBool(sub.writeUInt8(voxel.getColor()))
					}
				}
			}
		}
	}
	if (!node.name().empty()) {
		ws.writeUInt8(priv::CHUNK_ID_SHAPE_NAME_V6);
		ws.writePascalStringUInt8(node.name());
	}
	return savePointNodes(sceneGraph, node, ws);
}

bool CubzhFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	stream->write("CUBZH!", 6);
	wrapBool(stream->writeUInt32(6)) // version
	wrapBool(stream->writeUInt8(1))	 // zip compression
	const int64_t totalSizePos = stream->pos();
	wrapBool(stream->writeUInt32(0)) // total size is written at the end
	const int64_t afterHeaderPos = stream->pos();

	ThumbnailContext thumbnailCtx;
	thumbnailCtx.outputSize = glm::ivec2(128);
	const image::ImagePtr &image = createThumbnail(sceneGraph, ctx.thumbnailCreator, thumbnailCtx);
	if (image && image->isLoaded()) {
		WriteChunkStream ws(priv::CHUNK_ID_PREVIEW, *stream);
		image->writePNG(ws);
	}

	{
		WriteChunkStream ws(priv::CHUNK_ID_PALETTE_V6, *stream);
		const palette::Palette &palette = sceneGraph.firstPalette();
		const uint8_t colorCount = palette.colorCount();
		ws.writeUInt8(colorCount);
		for (uint8_t i = 0; i < colorCount; ++i) {
			const core::RGBA rgba = palette.color(i);
			wrapBool(ws.writeUInt8(rgba.r))
			wrapBool(ws.writeUInt8(rgba.g))
			wrapBool(ws.writeUInt8(rgba.b))
			wrapBool(ws.writeUInt8(rgba.a))
		}
		for (uint8_t i = 0; i < colorCount; ++i) {
			wrapBool(ws.writeBool(palette.hasEmit(i)))
		}
	}
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (node.isAnyModelNode()) {
			wrapBool(saveModelNode(sceneGraph, node, stream))
		}
	}

	const uint32_t totalSize = stream->size() - afterHeaderPos;
	if (stream->seek(totalSizePos) == -1) {
		Log::error("Failed to seek to the total size position in the header");
		return false;
	}
	wrapBool(stream->writeUInt32(totalSize))
	stream->seek(0, SEEK_END);

	return saveAnimations(sceneGraph, filename, archive, ctx);
}

static scenegraph::InterpolationType toInterpolationType(const std::string &type) {
	for (int i = 0; i < lengthof(scenegraph::InterpolationTypeStr); ++i) {
		if (type == scenegraph::InterpolationTypeStr[i]) {
			return (scenegraph::InterpolationType)i;
		}
	}
	return scenegraph::InterpolationType::Max;
}

bool CubzhFormat::saveAnimations(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								 const io::ArchivePtr &archive, const SaveContext &ctx) const {
	if (!sceneGraph.hasAnimations()) {
		return true;
	}
	const core::String animationFilename = filename + ".json";
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(animationFilename));
	if (!stream) {
		Log::error("Could not open file %s", animationFilename.c_str());
		return false;
	}
	const core::String basename = core::string::extractFilename(filename);
	// TODO: VOXELFORMAT: animation settings are not correct
	int fps = 12;
	int loopStart = 0;
	int loopEnd = 0;
	int maxTime = 35;
	const core::String shapeName = "author." + basename;

	if (!stream->writeString("{\n", false)) {
		Log::error("Failed to write to file '%s'", animationFilename.c_str());
		return false;
	}
	stream->writeString("\t\"animations\": {\n", false);

	for (const core::String &animation : sceneGraph.animations()) {
		stream->writeStringFormat(false, "\t\t\"%s\": {\n", animation.c_str());
		stream->writeStringFormat(false, "\t\t\t\"playSpeed\": %i,\n", fps);
		stream->writeStringFormat(false, "\t\t\t\"loopStart\": %i,\n", loopStart);
		stream->writeStringFormat(false, "\t\t\t\"loopEnd\": %i,\n", loopEnd);
		stream->writeStringFormat(false, "\t\t\t\"maxTime\": %i,\n", maxTime);
		stream->writeString("\t\t\t\"shapes\": {\n", false);
		bool firstNode = true;
		for (const auto &entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (!node.isAnyModelNode()) {
				continue;
			}
			if (firstNode) {
				firstNode = false;
			} else {
				stream->writeString(",\n", false);
			}
			stream->writeStringFormat(false, "\t\t\t\t\"%s\": {\n", node.name().c_str());
			stream->writeStringFormat(false, "\t\t\t\t\t\"name\": \"%s\",\n", node.name().c_str());
			stream->writeString("\t\t\t\t\t\"frames\": {\n", false);
			bool firstKeyFrame = true;
			for (const scenegraph::SceneGraphKeyFrame &keyframe : node.keyFrames(animation)) {
				const scenegraph::SceneGraphTransform &transform = keyframe.transform();
				const glm::vec3 &translation = transform.localTranslation();
				const glm::vec3 &eulerAngles = glm::eulerAngles(transform.localOrientation());
				const glm::quat &rotation = transform.localOrientation();
				if (firstKeyFrame) {
					firstKeyFrame = false;
				} else {
					stream->writeString(",\n", false);
				}
				stream->writeStringFormat(false, "\t\t\t\t\t\t\"%i_\": {\n", keyframe.frameIdx);
				stream->writeString("\t\t\t\t\t\t\t\"position\": {\n", false);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_x\": %f,\n", translation.x);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_y\": %f,\n", translation.y);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_z\": %f\n", translation.z);
				stream->writeString("\t\t\t\t\t\t\t},\n", false);
				stream->writeString("\t\t\t\t\t\t\t\"rotation\": {\n", false);
				stream->writeString("\t\t\t\t\t\t\t\t\"_edirty\": false,\n", false);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_ex\": %f,\n", eulerAngles.x);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_ey\": %f,\n", eulerAngles.y);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_ez\": %f,\n", eulerAngles.z);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_x\": %f,\n", rotation.x);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_y\": %f,\n", rotation.y);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_z\": %f,\n", rotation.z);
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\t\"_w\": %f\n", rotation.w);
				stream->writeString("\t\t\t\t\t\t\t},\n", false);

				core::String interpolation = scenegraph::InterpolationTypeStr[(int)keyframe.interpolation];
				interpolation = interpolation.toLower();
				stream->writeStringFormat(false, "\t\t\t\t\t\t\t\"interpolation\": \"%s\"\n", interpolation.c_str());
				stream->writeString("\t\t\t\t\t\t}", false);
			}
			stream->writeString("\n\t\t\t\t\t}\n\t\t\t\t}", false);
		}
		stream->writeString("\n", false);
	}
	stream->writeString("\t\t\t}\n\t\t}\n\t},\n", false);
	stream->writeStringFormat(false, "\t\"shape\": \"%s\",\n", shapeName.c_str());
	stream->writeString("\t\"shapeType\": \"shape\"\n", false);
	stream->writeString("}\n", false);
	return true;
}

bool CubzhFormat::loadAnimations(const core::String &filename, const io::ArchivePtr &archive,
								 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) const {
	const core::String animationFilename = filename + ".json";
	if (!archive->exists(animationFilename)) {
		return true;
	}
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(animationFilename));
	if (!stream) {
		Log::error("Could not open file '%s'", animationFilename.c_str());
		return false;
	}

	core::String jsonStr;
	if (!stream->readString(stream->size(), jsonStr)) {
		Log::error("Failed to read file '%s'", animationFilename.c_str());
		return false;
	}

	nlohmann::json j = nlohmann::json::parse(jsonStr, nullptr, false, true);

	if (j.contains("animations")) {
		const auto &animations = j.at("animations");
		for (const auto &animation : animations.items()) {
			const auto &key = animation.key();
			sceneGraph.addAnimation(key.c_str());
			sceneGraph.setAnimation(key.c_str());
			const auto &animationObject = animation.value();
			if (!animationObject.contains("shapes")) {
				continue;
			}
			const auto &shapes = animationObject.at("shapes");
			for (const auto &shape : shapes.items()) {
				const core::String name = shape.key().c_str();
				if (scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName(name.c_str())) {
					const auto &shapeObject = shape.value();
					if (!shapeObject.contains("frames")) {
						continue;
					}
					const auto &frames = shapeObject.at("frames");
					for (const auto &frame : frames.items()) {
						const scenegraph::FrameIndex frameIdx = core::string::toInt(frame.key().c_str());
						scenegraph::KeyFrameIndex keyFrameIdx = node->addKeyFrame(frameIdx);
						if (keyFrameIdx == InvalidKeyFrame) {
							keyFrameIdx = node->keyFrameForFrame(frameIdx);
						}
						if (keyFrameIdx == InvalidKeyFrame) {
							Log::error("Failed to add key frame %d to node %s", frameIdx, name.c_str());
							return false;
						}
						scenegraph::SceneGraphKeyFrame &keyFrame = node->keyFrame(keyFrameIdx);

						glm::vec3 localTranslation{0.0f};
						glm::quat localOrientation = glm::quat_identity<float, glm::defaultp>();
						const auto &frameObject = frame.value();
						if (frameObject.contains("position")) {
							const auto &position = frameObject.at("position");
							localTranslation.x = position.at("_x").get<float>();
							localTranslation.y = position.at("_y").get<float>();
							localTranslation.z = position.at("_z").get<float>();
						}
						if (frameObject.contains("rotation")) {
							const auto &rotation = frameObject.at("rotation");
							localOrientation.x = rotation.at("_x").get<float>();
							localOrientation.y = rotation.at("_y").get<float>();
							localOrientation.z = rotation.at("_z").get<float>();
							localOrientation.w = rotation.at("_w").get<float>();
						}
						if (frameObject.contains("interpolation")) {
							keyFrame.interpolation = toInterpolationType(frameObject.at("interpolation"));
							if (keyFrame.interpolation == scenegraph::InterpolationType::Max) {
								Log::error("Invalid interpolation type");
								keyFrame.interpolation = scenegraph::InterpolationType::Linear;
							}
						}
						scenegraph::SceneGraphTransform &transform = keyFrame.transform();
						transform.setLocalTranslation(localTranslation);
						transform.setLocalOrientation(localOrientation);
					}
				}
			}
		}
	}
	return true;
}

#undef wrapBool

} // namespace voxelformat
