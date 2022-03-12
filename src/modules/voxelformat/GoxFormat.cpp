/**
 * @file
 */

#include "GoxFormat.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "math/Axis.h"
#include "math/Math.h"
#include "voxel/MaterialColor.h"
#include "voxel/Voxel.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxel {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load gox file: Failure at " CORE_STRINGIFY(read));                                       \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Could not load gox file: Failure at " CORE_STRINGIFY(read));                                       \
		return false;                                                                                                  \
	}

#define wrapImg(read)                                                                                                  \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load gox file: Failure at " CORE_STRINGIFY(read));                                       \
		return image::ImagePtr();                                                                                      \
	}

#define wrapSave(write)                                                                                                \
	if ((write) == false) {                                                                                            \
		Log::error("Could not save gox file: " CORE_STRINGIFY(write) " failed");                                       \
		return false;                                                                                                  \
	}

class GoxScopedChunkWriter {
private:
	io::SeekableWriteStream& _stream;
	int64_t _chunkSizePos;
	uint32_t _chunkId;
public:
	GoxScopedChunkWriter(io::SeekableWriteStream& stream, uint32_t chunkId) : _stream(stream), _chunkId(chunkId) {
		uint8_t buf[4];
		FourCCRev(buf, chunkId);
		Log::debug("Saving %c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
		stream.writeUInt32(chunkId);
		_chunkSizePos = stream.pos();
		stream.writeUInt32(0);
	}

	~GoxScopedChunkWriter() {
		const int64_t chunkStart = _chunkSizePos + (int64_t)sizeof(uint32_t);
		const int64_t currentPos = _stream.pos();
		core_assert_msg(chunkStart <= currentPos, "%u should be <= %u", (uint32_t)chunkStart, (uint32_t)currentPos);
		const uint64_t chunkSize = currentPos - chunkStart;
		_stream.seek(_chunkSizePos);
		_stream.writeUInt32(chunkSize);
		_stream.seek(currentPos);
		_stream.writeUInt32(0); // CRC - not calculated
		uint8_t buf[4];
		FourCCRev(buf, _chunkId);
		Log::debug("Chunk size for %c%c%c%c: %i", buf[0], buf[1], buf[2], buf[3], (int)chunkSize);
	}
};

bool GoxFormat::loadChunk_Header(GoxChunk &c, io::SeekableReadStream &stream) {
	if (stream.eos()) {
		return false;
	}
	core_assert_msg(stream.remaining() >= 8, "stream should at least contain 8 more bytes, but only has %i", (int)stream.remaining());
	wrap(stream.readUInt32(c.type))
	wrap(stream.readInt32(c.length))
	c.streamStartPos = stream.pos();
	return true;
}

bool GoxFormat::loadChunk_ReadData(io::SeekableReadStream &stream, char *buff, int size) {
	if (size == 0) {
		return true;
	}
	if (stream.read(buff, size) == -1) {
		return false;
	}
	return true;
}

void GoxFormat::loadChunk_ValidateCRC(io::SeekableReadStream &stream) {
	uint32_t crc;
	stream.readUInt32(crc);
}

bool GoxFormat::loadChunk_DictEntry(const GoxChunk &c, io::SeekableReadStream &stream, char *key, char *value) {
	const int64_t endPos = c.streamStartPos + c.length;
	if (stream.pos() >= endPos) {
		return false;
	}
	if (stream.eos()) {
		Log::error("Unexpected end of stream in reading a dict entry");
		return false;
	}

	int keySize;
	wrap(stream.readInt32(keySize));
	if (keySize == 0) {
		Log::warn("Empty string for key in dict");
		return false;
	}
	if (keySize >= 256) {
		Log::error("Max size of 256 exceeded for dict key: %i", keySize);
		return false;
	}
	loadChunk_ReadData(stream, key, keySize);
	key[keySize] = '\0';

	int valueSize;
	wrap(stream.readInt32(valueSize));
	if (valueSize >= 256) {
		Log::error("Max size of 256 exceeded for dict value: %i", valueSize);
		return false;
	}
	// the values are floats, ints, strings, ... - but nevertheless the null byte for strings
	loadChunk_ReadData(stream, value, valueSize);
	value[valueSize] = '\0';

	Log::debug("Dict entry '%s'", key);
	return true;
}

image::ImagePtr GoxFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) {
	uint32_t magic;
	wrapImg(stream.readUInt32(magic))

	if (magic != FourCC('G', 'O', 'X', ' ')) {
		Log::error("Invalid magic");
		return image::ImagePtr();
	}

	uint32_t version;
	wrapImg(stream.readUInt32(version))

	if (version != 2) {
		Log::error("Unknown gox format version found: %u", version);
		return image::ImagePtr();
	}

	GoxChunk c;
	while (loadChunk_Header(c, stream)) {
		if (c.type == FourCC('B', 'L', '1', '6') || c.type == FourCC('L', 'A', 'Y', 'R')) {
			break;
		} else if (c.type == FourCC('P', 'R', 'E', 'V')) {
			uint8_t *png = (uint8_t *)core_malloc(c.length);
			wrapImg(loadChunk_ReadData(stream, (char *)png, c.length))
			image::ImagePtr img = image::createEmptyImage("gox-preview");
			img->load(png, c.length);
			core_free(png);
			return img;
		} else {
			stream.seek(c.length, SEEK_CUR);
		}
		loadChunk_ValidateCRC(stream);
	}
	return image::ImagePtr();
}

bool GoxFormat::loadChunk_LAYR(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	const int size = (int)sceneGraph.size();
	core::String name = core::string::format("layer %i", size);
	voxel::RawVolume *layerVolume = new voxel::RawVolume(voxel::Region(0, 0, 0, 1, 1, 1));
	uint32_t blockCount;

	wrap(stream.readUInt32(blockCount))
	Log::debug("Found LAYR chunk with %i blocks", blockCount);
	for (uint32_t i = 0; i < blockCount; ++i) {
		uint32_t index;
		wrap(stream.readUInt32(index))
		if (index > state.images.size()) {
			Log::error("Index out of bounds: %u", index);
			return false;
		}
		const image::ImagePtr &img = state.images[index];
		if (!img) {
			Log::error("Invalid image index: %u", index);
			return false;
		}
		Log::debug("LAYR references BL16 image with index %i", index);
		const uint8_t *rgba = img->data();
		int bpp = img->depth();
		int w = img->width();
		int h = img->height();
		core_assert(w == 64 && h == 64 && bpp == 4);

		int32_t x, y, z;
		wrap(stream.readInt32(x))
		wrap(stream.readInt32(y))
		wrap(stream.readInt32(z))
		// Previous version blocks pos.
		if (state.version == 1) {
			x -= 8;
			y -= 8;
			z -= 8;
		}
		wrap(stream.skip(4))
		voxel::Region blockRegion(x, z, y, x + (BlockSize - 1), z + (BlockSize - 1), y + (BlockSize - 1));
		voxel::RawVolume *blockVolume = new voxel::RawVolume(blockRegion);
		const uint8_t *v = rgba;
		bool empty = true;
		for (int y1 = blockRegion.getLowerY(); y1 <= blockRegion.getUpperY(); ++y1) {
			for (int z1 = blockRegion.getLowerZ(); z1 <= blockRegion.getUpperZ(); ++z1) {
				for (int x1 = blockRegion.getLowerX(); x1 <= blockRegion.getUpperX(); ++x1) {
					voxel::VoxelType voxelType = voxel::VoxelType::Generic;
					uint8_t index;
					if (v[3] == 0u) {
						voxelType = voxel::VoxelType::Air;
						index = 0;
					} else {
						const uint32_t color = core::Color::getRGBA(v[0], v[1], v[2], v[3]);
						index = findClosestIndex(color);
					}
					const voxel::Voxel voxel = voxel::createVoxel(voxelType, index);
					blockVolume->setVoxel(x1, y1, z1, voxel);
					if (!voxel::isAir(voxel.getMaterial())) {
						empty = false;
					}
					v += 4;
				}
			}
		}
		// this will remove empty blocks and the final volume might have a smaller region.
		// TODO: we should remove this once we have sparse volumes support
		if (!empty) {
			voxel::Region destReg(layerVolume->region());
			if (!destReg.containsRegion(blockRegion)) {
				destReg.accumulate(blockRegion);
				voxel::RawVolume *newVolume = new voxel::RawVolume(destReg);
				voxelutil::copyIntoRegion(*layerVolume, *newVolume, layerVolume->region());
				delete layerVolume;
				layerVolume = newVolume;
			}
			voxel::mergeVolumes(layerVolume, blockVolume, blockRegion, blockRegion);
		}
		delete blockVolume;
	}
	bool visible = true;
	char dictKey[256];
	char dictValue[256];
	SceneGraphNode node;
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue)) {
		if (!strcmp(dictKey, "name")) {
			// "name" 255 chars max
			name = dictValue;
			node.setProperty(dictKey, dictValue);
		} else if (!strcmp(dictKey, "visible")) {
			// "visible" (bool)
			visible = *dictValue;
		} else if (!strcmp(dictKey, "mat")) {
			// "mat" (4x4 matrix)
			SceneGraphTransform transform;
			io::MemoryReadStream stream(dictValue, sizeof(float) * 16);
			for (int i = 0; i < 16; ++i) {
				stream.readFloat(transform.mat[i / 4][i % 4]);
			}
			transform.updateFromMat();
			node.setTransform(0, transform, true);
		} else if (!strcmp(dictKey, "img-path") || !strcmp(dictKey, "id")) {
			// "img-path" layer texture path
			// "id" unique id
			node.setProperty(dictKey, dictValue);
		} else if (!strcmp(dictKey, "base_id") || !strcmp(dictKey, "material")) {
			// "base_id" int
			// "material" int (index)
			node.setProperty(dictKey, core::string::toString(*(const int32_t*)dictValue));
		} else if (!strcmp(dictKey, "box") || !strcmp(dictKey, "shape") || !strcmp(dictKey, "color")) {
			// "box" 4x4 bounding box float
			// "shape" layer layer - currently unsupported TODO
			// "color" 4xbyte
		}
	}
	// TODO: fix this properly - without mirroring
	node.setVolume(voxel::mirrorAxis(layerVolume, math::Axis::Z), true);
	node.setName(name);
	node.setVisible(visible);
	sceneGraph.emplace(core::move(node));
	delete layerVolume;
	return true;
}

bool GoxFormat::loadChunk_BL16(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	uint8_t* png = (uint8_t*)core_malloc(c.length);
	wrapBool(loadChunk_ReadData(stream, (char *)png, c.length))
	image::ImagePtr img = image::createEmptyImage("gox-voxeldata");
	bool success = img->load(png, c.length);
	core_free(png);
	if (!success) {
		Log::error("Failed to load png chunk");
		return false;
	}
	Log::debug("Found BL16 with index %i", (int)state.images.size());
	state.images.push_back(img);
	return true;
}

bool GoxFormat::loadChunk_MATE(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue)) {
		// "name" 127 chars max
		// "color" 4xfloat
		// "metallic" float
		// "roughness" float
		// "emission" 3xfloat
	}
	return true;
}

bool GoxFormat::loadChunk_CAMR(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Camera);
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue)) {
		if (!strcmp(dictKey, "name")) {
			// "name" 127 chars max
			node.setProperty(dictKey, dictValue);
		} else if (!strcmp(dictKey, "active")) {
			// "active" no value - active scene camera if this key is available
			node.setProperty(dictKey, "true");
		} else if (!strcmp(dictKey, "dist")) {
			// "dist" float
			node.setProperty(dictKey, core::string::toString(*(const float*)dictValue));
		} else if (!strcmp(dictKey, "ortho")) {
			// "ortho" bool
			node.setProperty(dictKey, *dictValue ? "true" : "false");
		} else if (!strcmp(dictKey, "mat")) {
			// "mat" 4x4 float
			SceneGraphTransform transform;
			io::MemoryReadStream stream(dictValue, sizeof(float) * 16);
			for (int i = 0; i < 16; ++i) {
				stream.readFloat(transform.mat[i / 4][i % 4]);
			}
			transform.updateFromMat();
			node.setTransform(0, transform, true);
		}
	}
	sceneGraph.emplace(core::move(node));
	return true;
}

bool GoxFormat::loadChunk_IMG(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue)) {
		// "box" 4x4 float bounding box
	}
	return true;
}

bool GoxFormat::loadChunk_LIGH(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue)) {
		// "pitch" float
		// "yaw" float
		// "intensity" float
		// "fixed" bool
		// "ambient" float
		// "shadow" float
	}
	return true;
}

bool GoxFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))

	if (magic != FourCC('G', 'O', 'X', ' ')) {
		Log::error("Invalid magic");
		return false;
	}

	State state;
	wrap(stream.readInt32(state.version))

	if (state.version > 2) {
		Log::error("Unknown gox format version found: %u", state.version);
		return false;
	}

	GoxChunk c;
	while (loadChunk_Header(c, stream)) {
		if (c.type == FourCC('B', 'L', '1', '6')) {
			wrapBool(loadChunk_BL16(state, c, stream, sceneGraph))
		} else if (c.type == FourCC('L', 'A', 'Y', 'R')) {
			wrapBool(loadChunk_LAYR(state, c, stream, sceneGraph))
		} else if (c.type == FourCC('C', 'A', 'M', 'R')) {
			wrapBool(loadChunk_CAMR(state, c, stream, sceneGraph))
		} else if (c.type == FourCC('M', 'A', 'T', 'E')) {
			wrapBool(loadChunk_MATE(state, c, stream, sceneGraph))
		} else if (c.type == FourCC('I', 'M', 'G', ' ')) {
			wrapBool(loadChunk_IMG(state, c, stream, sceneGraph))
		} else if (c.type == FourCC('L', 'I', 'G', 'H')) {
			wrapBool(loadChunk_LIGH(state, c, stream, sceneGraph))
		} else {
			stream.seek(c.length, SEEK_CUR);
		}
		loadChunk_ValidateCRC(stream);
	}
	return !sceneGraph.empty();
}

bool GoxFormat::saveChunk_DictEntry(io::SeekableWriteStream &stream, const char *key, const void *value, size_t valueSize) {
	const int keyLength = (int)SDL_strlen(key);
	wrapBool(stream.writeUInt32(keyLength))
	if (stream.write(key, keyLength) == -1) {
		Log::error("Failed to write dict entry key");
		return false;
	}
	wrapBool(stream.writeUInt32(valueSize))
	if (stream.write(value, valueSize) == -1) {
		Log::error("Failed to write dict entry value");
		return false;
	}
	return true;
}

bool GoxFormat::saveChunk_IMG(io::SeekableWriteStream& stream) {
	return true; // not used
}

bool GoxFormat::saveChunk_PREV(io::SeekableWriteStream& stream) {
	return true; // not used
}

bool GoxFormat::saveChunk_CAMR(io::SeekableWriteStream& stream) {
	return true; // not used
}

bool GoxFormat::saveChunk_LIGH(io::SeekableWriteStream& stream) {
	return true; // not used
}

bool GoxFormat::saveChunk_MATE(io::SeekableWriteStream& stream) {
	GoxScopedChunkWriter scoped(stream, FourCC('M', 'A', 'T', 'E'));
	const voxel::Palette& palette = voxel::getPalette();

	for (int i = 0; i < palette.colorCount; ++i) {
		const core::String& name = core::string::format("mat%i", i);
		const float value[3] = {0.0f, 0.0f, 0.0f};
		wrapBool(saveChunk_DictEntry(stream, "name", name.c_str(), name.size()))
		wrapBool(saveChunk_DictEntry(stream, "color", palette.colors[i]))
		wrapBool(saveChunk_DictEntry(stream, "metallic", value[0]))
		wrapBool(saveChunk_DictEntry(stream, "roughness", value[0]))
		wrapBool(saveChunk_DictEntry(stream, "emission", value))
	}
	return true;
}

bool GoxFormat::saveChunk_LAYR(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph, int numBlocks) {
	int blockUid = 0;
	int layerId = 0;
	for (const SceneGraphNode &node : sceneGraph) {
		const voxel::Region &region = node.region();
		glm::ivec3 mins, maxs;
		calcMinsMaxs(region, glm::ivec3(BlockSize), mins, maxs);

		GoxScopedChunkWriter scoped(stream, FourCC('L', 'A', 'Y', 'R'));
		int layerBlocks = 0;
		voxelutil::visitVolume(*node.volume(), voxel::Region(mins, maxs), BlockSize, BlockSize, BlockSize, [&] (int x, int y, int z, const voxel::Voxel &) {
			if (isEmptyBlock(node.volume(), glm::ivec3(BlockSize), x, y, z)) {
				return;
			}
			++layerBlocks;
		}, voxelutil::VisitAll());

		Log::debug("blocks: %i", layerBlocks);

		wrapBool(stream.writeUInt32(layerBlocks))

		for (int y = mins.y; y <= maxs.y; y += BlockSize) {
			for (int z = mins.z; z <= maxs.z; z += BlockSize) {
				for (int x = mins.x; x <= maxs.x; x += BlockSize) {
					if (isEmptyBlock(node.volume(), glm::ivec3(BlockSize), x, y, z)) {
						continue;
					}
					Log::debug("Saved LAYR chunk %i at %i:%i:%i", blockUid, x, y, z);
					wrapBool(stream.writeUInt32(blockUid++))
					wrapBool(stream.writeInt32(x))
					wrapBool(stream.writeInt32(z))
					wrapBool(stream.writeInt32(y))
					wrapBool(stream.writeUInt32(0))
					--layerBlocks;
					--numBlocks;
				}
			}
		}
		if (layerBlocks != 0) {
			Log::error("Invalid amount of layer blocks: %i", layerBlocks);
			return false;
		}
		wrapBool(saveChunk_DictEntry(stream, "name", node.name().c_str(), node.name().size()))
		glm::mat4 mat(1.0f);
		wrapBool(saveChunk_DictEntry(stream, "mat", (const uint8_t*)glm::value_ptr(mat), sizeof(mat)))
		wrapBool(saveChunk_DictEntry(stream, "id", layerId))
#if 0
		wrapBool(saveChunk_DictEntry(stream, "base_id", &layer->base_id))
		wrapBool(saveChunk_DictEntry(stream, "material", &material_idx))
#endif
		wrapBool(saveChunk_DictEntry(stream, "visible", node.visible()))

		++layerId;
	}
	if (numBlocks != 0) {
		Log::error("Invalid amount of blocks");
		return false;
	}
	return true;
}

bool GoxFormat::saveChunk_BL16(io::SeekableWriteStream& stream, const SceneGraph &sceneGraph, int &blocks) {
	blocks = 0;
	for (const SceneGraphNode &node : sceneGraph) {
		const voxel::Region &region = node.region();
		glm::ivec3 mins, maxs;
		calcMinsMaxs(region, glm::ivec3(BlockSize), mins, maxs);

		// TODO: fix this properly - without mirroring
		voxel::RawVolume *mirrored = voxel::mirrorAxis(node.volume(), math::Axis::Z);
		for (int by = mins.y; by <= maxs.y; by += BlockSize) {
			for (int bz = mins.z; bz <= maxs.z; bz += BlockSize) {
				for (int bx = mins.x; bx <= maxs.x; bx += BlockSize) {
					if (isEmptyBlock(mirrored, glm::ivec3(BlockSize), bx, by, bz)) {
						continue;
					}
					GoxScopedChunkWriter scoped(stream, FourCC('B', 'L', '1', '6'));
					const voxel::Region blockRegion(bx, by, bz, bx + BlockSize - 1, by + BlockSize - 1, bz + BlockSize - 1);
					const size_t size = (size_t)BlockSize * BlockSize * BlockSize * 4;
					uint32_t *data = (uint32_t*)core_malloc(size);
					int offset = 0;
					const voxel::Palette& palette = voxel::getPalette();
					voxelutil::visitVolume(*mirrored, blockRegion, [&](int, int, int, const voxel::Voxel& voxel) {
						if (voxel::isAir(voxel.getMaterial())) {
							data[offset++] = 0;
						} else {
							data[offset++] = palette.colors[voxel.getColor()];
						}
					}, voxelutil::VisitAll(), voxelutil::VisitorOrder::YZX);

					int pngSize = 0;
					uint8_t *png = image::createPng(data, 64, 64, 4, &pngSize);
					core_free(data);

					if (stream.write(png, pngSize) == -1) {
						Log::error("Could not write png into gox stream");
						core_free(png);
						delete mirrored;
						return false;
					}
					core_free(png);
					Log::debug("Saved BL16 chunk %i with a pngsize of %i", blocks, pngSize);
					++blocks;
				}
			}
		}
		delete mirrored;
	}
	return true;
}

bool GoxFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream) {
	wrapSave(stream.writeUInt32(FourCC('G', 'O', 'X', ' ')))
	wrapSave(stream.writeUInt32(2))

	wrapBool(saveChunk_IMG(stream))
	wrapBool(saveChunk_PREV(stream))
	int blocks = 0;
	wrapBool(saveChunk_BL16(stream, sceneGraph, blocks))
	wrapBool(saveChunk_MATE(stream))
	wrapBool(saveChunk_LAYR(stream, sceneGraph, blocks))
	wrapBool(saveChunk_CAMR(stream))
	wrapBool(saveChunk_LIGH(stream))

	return true;
}

#undef wrapBool
#undef wrapImg
#undef wrap
#undef wrapSave

} // namespace voxel
