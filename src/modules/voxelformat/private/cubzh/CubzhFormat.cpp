/**
 * @file
 */

#include "CubzhFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load 3zh file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load 3zh file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

namespace priv {
enum ChunkId {
	CHUNK_ID_PREVIEW = 1,
	CHUNK_ID_PALETTE_V5 = 2,
	CHUNK_ID_SHAPE_V5 = 5,
	CHUNK_ID_SHAPE_SIZE_V5 = 6,
	CHUNK_ID_SHAPE_BLOCKS_V5 = 7,
	CHUNK_ID_SHAPE_POINT_V5 = 8,
	CHUNK_ID_PALETTE_LEGACY_V6 = 2,
	CHUNK_ID_SHAPE_V6 = 3,
	CHUNK_ID_SHAPE_SIZE_V6 = 4,
	CHUNK_ID_SHAPE_BLOCKS_V6 = 5,
	CHUNK_ID_SHAPE_POINT_V6 = 6,
	CHUNK_ID_SHAPE_BAKED_LIGHTING_V6 = 7,
	CHUNK_ID_SHAPE_POINT_ROTATION_V6 = 8,
	CHUNK_ID_CAMERA_V6 = 10,
	CHUNK_ID_DIRECTIONAL_LIGHT_V6 = 11,
	CHUNK_ID_SOURCE_METADATA_V6 = 12,
	CHUNK_ID_GENERAL_RENDERING_OPTIONS_V6 = 14,
	CHUNK_ID_PALETTE_ID_V6 = 15,
	CHUNK_ID_PALETTE_V6 = 16,
	CHUNK_ID_SHAPE_ID_V6 = 17,
	CHUNK_ID_SHAPE_NAME_V6 = 18,
	CHUNK_ID_SHAPE_PARENT_ID_V6 = 19,
	CHUNK_ID_SHAPE_TRANSFORM_V6 = 20,
	CHUNK_ID_SHAPE_PIVOT_V6 = 21,
	CHUNK_ID_SHAPE_PALETTE_V6 = 22,
	CHUNK_ID_OBJECT_COLLISION_BOX_V6 = 23,
	CHUNK_ID_OBJECT_IS_HIDDEN_V6 = 24,
	CHUNK_ID_MIN = 1,
	CHUNK_ID_MAX = 24
};

} // namespace priv

bool CubzhFormat::Chunk::supportsCompression() const {
	return chunkId == priv::CHUNK_ID_PALETTE_V6 || chunkId == priv::CHUNK_ID_SHAPE_V6 ||
		   chunkId == priv::CHUNK_ID_PALETTE_LEGACY_V6 || chunkId == priv::CHUNK_ID_PALETTE_ID_V6;
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
	core_assert_msg(dataSize <= (size_t)remaining(), "requested to read %d bytes, but only %d are left", (int)dataSize,
					(int)remaining());
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
	if (header.version == 6 && chunk.supportsCompression()) {
		Log::debug("skip additional header bytes for compressed chunk");
		stream.skipDelta(5);
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
	if (header.version != 5 && header.version != 6) {
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

bool CubzhFormat::loadPalette(const core::String &filename, const Header &header, const Chunk &chunk,
							  io::ReadStream &stream, voxel::Palette &palette) const {
	const bool legacy =
		header.version == 5 || (header.version == 6 && chunk.chunkId == priv::CHUNK_ID_PALETTE_LEGACY_V6);
	uint8_t colorCount;
	if (legacy) {
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
	} else {
		wrap(stream.readUInt8(colorCount))
	}
	Log::debug("Palette with %d colors", colorCount);

	palette.setSize(colorCount);
	for (uint8_t i = 0; i < colorCount; ++i) {
		uint8_t r, g, b, a;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))
		wrap(stream.readUInt8(a))
		palette.color(i) = core::RGBA(r, g, b, a);
	}
	for (uint8_t i = 0; i < colorCount; ++i) {
		const bool emissive = stream.readBool();
		if (emissive) {
			palette.glowColor(i) = palette.color(i);
		}
	}
	return true;
}

bool CubzhFormat::loadShape5(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
							 scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette,
							 const LoadContext &ctx) const {
	uint16_t width = 0, depth = 0, height = 0;
	scenegraph::SceneGraphNode node;

	Chunk chunk;
	wrapBool(loadSubChunkHeader(stream, chunk))
	switch (chunk.chunkId) {
	case priv::CHUNK_ID_SHAPE_SIZE_V5:
		wrap(stream.readUInt16(width))
		wrap(stream.readUInt16(height))
		wrap(stream.readUInt16(depth))
		break;
	case priv::CHUNK_ID_SHAPE_BLOCKS_V5: {
		if (width == 0) {
			// TODO: support for blocks-before-size-chunk loading via stream seeking
			Log::error("Size chunk not yet loaded");
			return false;
		}
		uint32_t voxelCount = (uint32_t)width * (uint32_t)height * (uint32_t)depth;
		if (voxelCount * sizeof(uint8_t) != chunk.chunkSize) {
			Log::error("Invalid size for blocks chunk: %i", chunk.chunkSize);
			return false;
		}
		const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
		if (!region.isValid()) {
			Log::error("Invalid region: %i:%i:%i", width, height, depth);
			return false;
		}

		voxel::RawVolume *volume = new voxel::RawVolume(region);
		node.setVolume(volume, true);
		for (uint16_t x = 0; x < width; x++) { // shape blocks
			for (uint16_t y = 0; y < height; y++) {
				for (uint16_t z = 0; z < depth; z++) {
					uint8_t index;
					wrap(stream.readUInt8(index))
					if (index == emptyPaletteIndex()) {
						continue;
					}
					const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
					volume->setVoxel(x, y, z, voxel);
				}
			}
		}
		break;
	}
	case priv::CHUNK_ID_SHAPE_POINT_V5: {
		core::String name;
		wrapBool(stream.readPascalStringUInt8(name))
		float f3x, f3y, f3z;
		wrap(stream.readFloat(f3x))
		wrap(stream.readFloat(f3y))
		wrap(stream.readFloat(f3z))
		node.setProperty(name, core::string::format("%f:%f:%f", f3x, f3y, f3z));
		break;
	}
	default:
		wrapBool(loadSkipSubChunk(chunk, stream))
		break;
	}
	node.setName(filename);
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool CubzhFormat::loadVersion5(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
							   const LoadContext &ctx) const {
	while (!stream.eos()) {
		Chunk chunk;
		wrapBool(loadChunkHeader(header, stream, chunk))
		switch (chunk.chunkId) {
		case priv::CHUNK_ID_PALETTE_V5:
			if (!loadPalette(filename, header, chunk, stream, palette)) {
				return false;
			}
			break;
		case priv::CHUNK_ID_SHAPE_V5:
			if (!loadShape5(filename, header, stream, sceneGraph, palette, ctx)) {
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

bool CubzhFormat::loadChunkHeader(const Header &header, io::ReadStream &stream, Chunk &chunk) const {
	wrap(stream.readUInt8(chunk.chunkId))
	wrap(stream.readUInt32(chunk.chunkSize))
	Log::debug("Chunk id %u with size %u", chunk.chunkId, chunk.chunkSize);
	if (header.version == 6 && chunk.supportsCompression()) {
		wrap(stream.readUInt8(chunk.compressed))
		wrap(stream.readUInt32(chunk.uncompressedSize))
		Log::debug("Compressed: %u", chunk.compressed);
		Log::debug("Uncompressed size: %u", chunk.uncompressedSize);
	}
	return true;
}

bool CubzhFormat::loadSubChunkHeader(io::ReadStream &stream, Chunk &chunk) const {
	wrap(stream.readUInt8(chunk.chunkId))
	wrap(stream.readUInt32(chunk.chunkSize))
	Log::debug("Subchunk id %u with size %u", chunk.chunkId, chunk.chunkSize);
	return true;
}

bool CubzhFormat::loadShape6(const core::String &filename, const Header &header, CubzhReadStream &stream,
							 scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette,
							 const LoadContext &ctx) const {
	uint16_t width = 0, depth = 0, height = 0;
	scenegraph::SceneGraphNode node;
	node.setName(filename);
	uint16_t shapeId = 1;
	uint16_t parentShapeId = 0;
	glm::vec3 pivot{0.5f}; // default is center of shape
	glm::vec3 pos{0};
	glm::vec3 eulerAngles{0};
	glm::vec3 scale{1};
	voxel::Palette nodePalette = palette;
	bool hasPivot = false;

	while (!stream.eos()) {
		Log::debug("Remaining sub stream data: %d", (int)stream.remaining());
		Chunk chunk;
		wrapBool(loadSubChunkHeader(stream, chunk))
		switch (chunk.chunkId) {
		case priv::CHUNK_ID_SHAPE_ID_V6:
			wrap(stream.readUInt16(shapeId))
			break;
		case priv::CHUNK_ID_SHAPE_PARENT_ID_V6:
			wrap(stream.readUInt16(parentShapeId))
			break;
		case priv::CHUNK_ID_SHAPE_TRANSFORM_V6: {
			wrap(stream.readFloat(pos.x))
			wrap(stream.readFloat(pos.y))
			wrap(stream.readFloat(pos.z))
			wrap(stream.readFloat(eulerAngles.x))
			wrap(stream.readFloat(eulerAngles.y))
			wrap(stream.readFloat(eulerAngles.z))
			wrap(stream.readFloat(scale.x))
			wrap(stream.readFloat(scale.y))
			wrap(stream.readFloat(scale.z))
			break;
		}
		case priv::CHUNK_ID_SHAPE_PIVOT_V6: {
			wrap(stream.readFloat(pivot.x))
			wrap(stream.readFloat(pivot.y))
			wrap(stream.readFloat(pivot.z))
			hasPivot = true;
			Log::debug("pivot: %f:%f:%f", pivot.x, pivot.y, pivot.z);
			break;
		}
		case priv::CHUNK_ID_SHAPE_PALETTE_V6: {
			wrapBool(loadPalette(filename, header, chunk, stream, nodePalette))
			break;
		}
		case priv::CHUNK_ID_OBJECT_COLLISION_BOX_V6: {
			glm::vec3 mins;
			glm::vec3 maxs;
			wrap(stream.readFloat(mins.x))
			wrap(stream.readFloat(mins.y))
			wrap(stream.readFloat(mins.z))
			wrap(stream.readFloat(maxs.x))
			wrap(stream.readFloat(maxs.y))
			wrap(stream.readFloat(maxs.z))
			break;
		}
		case priv::CHUNK_ID_OBJECT_IS_HIDDEN_V6: {
			node.setVisible(!stream.readBool());
			break;
		}
		case priv::CHUNK_ID_SHAPE_NAME_V6: {
			core::String name;
			stream.readPascalStringUInt8(name);
			node.setName(name);
			break;
		}
		case priv::CHUNK_ID_SHAPE_SIZE_V6:
			wrap(stream.readUInt16(width))
			wrap(stream.readUInt16(height))
			wrap(stream.readUInt16(depth))
			break;
		case priv::CHUNK_ID_SHAPE_BLOCKS_V6: {
			if (width == 0) {
				// TODO: support for blocks-before-size-chunk loading
				Log::error("Size chunk not yet loaded");
				return false;
			}
			uint32_t voxelCount = (uint32_t)width * (uint32_t)height * (uint32_t)depth;
			if (voxelCount * sizeof(uint8_t) != chunk.chunkSize) {
				Log::error("Invalid size for blocks chunk: %i", chunk.chunkSize);
				return false;
			}
			const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
			if (!region.isValid()) {
				Log::error("Invalid region: %i:%i:%i", width, height, depth);
				return false;
			}

			voxel::RawVolume *volume = new voxel::RawVolume(region);
			node.setVolume(volume, true);
			for (uint16_t x = 0; x < width; x++) { // shape blocks
				for (uint16_t y = 0; y < height; y++) {
					for (uint16_t z = 0; z < depth; z++) {
						uint8_t index;
						wrap(stream.readUInt8(index))
						if (index == emptyPaletteIndex()) {
							continue;
						}
						const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
						volume->setVoxel(x, y, z, voxel);
					}
				}
			}
			break;
		}
		case priv::CHUNK_ID_SHAPE_POINT_V6: {
			core::String name;
			wrapBool(stream.readPascalStringUInt8(name))
			float f3x, f3y, f3z;
			wrap(stream.readFloat(f3x))
			wrap(stream.readFloat(f3y))
			wrap(stream.readFloat(f3z))
			node.setProperty(name, core::string::format("%f:%f:%f", f3x, f3y, f3z));
			break;
		}
		case priv::CHUNK_ID_SHAPE_POINT_ROTATION_V6: {
			core::String name;
			wrapBool(stream.readPascalStringUInt8(name))
			glm::vec3 poiPos;
			wrap(stream.readFloat(poiPos.x))
			wrap(stream.readFloat(poiPos.y))
			wrap(stream.readFloat(poiPos.z))
			break;
		}
		case priv::CHUNK_ID_SHAPE_BAKED_LIGHTING_V6:
		default:
			wrapBool(loadSkipSubChunk(chunk, stream))
			break;
		}
	}
	scenegraph::SceneGraphTransform transform;
	transform.setLocalTranslation(pos);
	transform.setLocalOrientation(glm::quat(eulerAngles));
	transform.setLocalScale(scale);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	if (hasPivot) {
		pivot.x /= (float)width;
		pivot.y /= (float)height;
		pivot.z /= (float)depth;
	}
	node.setPivot(pivot);
	node.setPalette(nodePalette);
	// TODO: parent support
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool CubzhFormat::loadVersion6(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
							   const LoadContext &ctx) const {
	while (!stream.eos()) {
		Log::debug("Remaining stream data: %d", (int)stream.remaining());
		Chunk chunk;
		wrapBool(loadChunkHeader(header, stream, chunk))
		if (chunk.chunkId < priv::CHUNK_ID_MIN || chunk.chunkId > priv::CHUNK_ID_MAX) {
			Log::warn("Invalid chunk id found: %u", chunk.chunkId);
			break;
		}
		switch (chunk.chunkId) {
		case priv::CHUNK_ID_PALETTE_V6:
		case priv::CHUNK_ID_PALETTE_LEGACY_V6: {
			Log::debug("load palette");
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadPalette(filename, header, chunk, zhs, palette))
			break;
		}
		case priv::CHUNK_ID_SHAPE_V6: {
			Log::debug("load shape");
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadShape6(filename, header, zhs, sceneGraph, palette, ctx))
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
			wrapBool(loadSkipChunk(header, chunk, stream))
			break;
		}
	}

	return true;
}

bool CubzhFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
									scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
									const LoadContext &ctx) {
	Header header;
	wrapBool(loadHeader(stream, header))
	if (header.version == 5) {
		return loadVersion5(filename, header, stream, sceneGraph, palette, ctx);
	} else {
		return loadVersion6(filename, header, stream, sceneGraph, palette, ctx);
	}
	return false;
}

size_t CubzhFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
								const LoadContext &ctx) {
	Header header;
	wrapBool(loadHeader(stream, header))
	while (!stream.eos()) {
		Chunk chunk;
		wrapBool(loadChunkHeader(header, stream, chunk))
		if (header.version == 5 && chunk.chunkId == priv::CHUNK_ID_PALETTE_V5) {
			wrapBool(loadPalette(filename, header, chunk, stream, palette))
		} else if (header.version == 6 && chunk.chunkId == priv::CHUNK_ID_PALETTE_V6) {
			CubzhReadStream zhs(header, chunk, stream);
			wrapBool(loadPalette(filename, header, chunk, zhs, palette))
		} else {
			wrapBool(loadSkipChunk(header, chunk, stream))
		}
	}
	return palette.size();
}

image::ImagePtr CubzhFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
											const LoadContext &ctx) {
	Header header;
	if (!loadHeader(stream, header)) {
		Log::error("Failed to read header");
		return image::ImagePtr();
	}
	while (!stream.eos()) {
		Chunk chunk;
		if (!loadChunkHeader(header, stream, chunk)) {
			return image::ImagePtr();
		}
		if (chunk.chunkId == priv::CHUNK_ID_PREVIEW) {
			image::ImagePtr img = image::createEmptyImage(core::string::extractFilename(filename) + ".png");
			img->load(stream, chunk.chunkSize);
			return img;
		}
		if (!loadSkipChunk(header, chunk, stream)) {
			Log::error("Failed to skip chunk %d with size %d", chunk.chunkId, chunk.chunkSize);
			break;
		}
	}
	return image::ImagePtr();
}

#undef wrap
#undef wrapBool

bool CubzhFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 io::SeekableWriteStream &stream, const SaveContext &ctx) {
	Log::error("Saving is not yet supported");
	return false;
}

} // namespace voxelformat
