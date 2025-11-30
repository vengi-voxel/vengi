/**
 * @file
 */

#include "GoxFormat.h"
#include "app/Async.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/concurrent/Atomic.h"
#include "image/Image.h"
#include "image/ImageType.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/StreamUtil.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxelformat {

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
	io::SeekableWriteStream &_stream;
	int64_t _chunkSizePos;
	uint32_t _chunkId;

public:
	GoxScopedChunkWriter(io::SeekableWriteStream &stream, uint32_t chunkId) : _stream(stream), _chunkId(chunkId) {
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
		_stream.writeUInt32((uint32_t)chunkSize);
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
	core_assert_msg(stream.remaining() >= 8, "stream should at least contain 8 more bytes, but only has %i",
					(int)stream.remaining());
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

bool GoxFormat::loadChunk_DictEntry(const GoxChunk &c, io::SeekableReadStream &stream, char *key, char *value, int &valueSize) {
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

image::ImagePtr GoxFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return image::ImagePtr();
	}
	uint32_t magic;
	wrapImg(stream->readUInt32(magic))

	if (magic != FourCC('G', 'O', 'X', ' ')) {
		Log::error("Invalid gox magic");
		return image::ImagePtr();
	}

	uint32_t version;
	wrapImg(stream->readUInt32(version))

	if (version != 2) {
		Log::error("Unknown gox format version found: %u", version);
		return image::ImagePtr();
	}

	GoxChunk c;
	while (loadChunk_Header(c, *stream)) {
		if (c.type == FourCC('P', 'R', 'E', 'V')) {
			image::ImagePtr img = image::createEmptyImage(core::string::extractFilename(filename) + ".png");
			img->load(image::ImageType::PNG, *stream, c.length);
			return img;
		} else {
			stream->seek(c.length, SEEK_CUR);
		}
		loadChunk_ValidateCRC(*stream);
	}
	return image::ImagePtr();
}

bool GoxFormat::loadChunk_LAYR(State &state, const GoxChunk &c, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette) {
	const int size = (int)sceneGraph.size();
	voxel::RawVolume *modelVolume = new voxel::RawVolume(voxel::Region(0, 0, 0, 1, 1, 1));
	uint32_t blockCount;

	if ((stream.readUInt32(blockCount)) != 0) {
		Log::error("Could not load gox file: Failed to read blockCount");
		delete modelVolume;
		return false;
	}
	Log::debug("Found LAYR chunk with %i blocks", blockCount);
	for (uint32_t i = 0; i < blockCount; ++i) {
		uint32_t index;
		if ((stream.readUInt32(index)) != 0) {
			Log::error("Could not load gox file: Failure to read block index");
			delete modelVolume;
			return false;
		}
		if (index > state.images.size()) {
			Log::error("Index out of bounds: %u", index);
			delete modelVolume;
			return false;
		}
		const image::ImagePtr &img = state.images[index];
		if (!img) {
			Log::error("Invalid image index: %u", index);
			delete modelVolume;
			return false;
		}
		Log::debug("LAYR references BL16 image with index %i", index);
		const uint8_t *rgba = img->data();
		int bpp = img->components();
		int w = img->width();
		int h = img->height();
		core_assert(w == 64 && h == 64 && bpp == 4);
		(void)bpp;(void)w;(void)h;

		int32_t x, y, z;
		if (stream.readInt32(x) != 0) {
			Log::error("Could not load gox file: Failure to read block coordinate");
			delete modelVolume;
			return false;
		}
		if (stream.readInt32(y) != 0) {
			Log::error("Could not load gox file: Failure to read block coordinate");
			delete modelVolume;
			return false;
		}
		if (stream.readInt32(z) != 0) {
			Log::error("Could not load gox file: Failure to read block coordinate");
			delete modelVolume;
			return false;
		}
		// Previous version blocks pos.
		if (state.version == 1) {
			x -= 8;
			y -= 8;
			z -= 8;
		}

		if (stream.skip(4) == -1) {
			Log::error("Could not load gox file: Failed to skip");
			delete modelVolume;
			return false;
		}
		const voxel::Region blockRegion(x, z, y, x + (BlockSize - 1), z + (BlockSize - 1), y + (BlockSize - 1));
		core_assert(blockRegion.isValid());
		voxel::RawVolume *blockVolume = new voxel::RawVolume(blockRegion);
		core::AtomicBool empty {true};
		palette::PaletteLookup palLookup(palette);
		auto fn = [blockVolume, rgba, &palLookup, &palette, x, y, z, this, &empty](int start, int end) {
			voxel::RawVolume::Sampler sampler(blockVolume);
			sampler.setPosition(x, z + start, y);
			for (int z1 = start; z1 < end; ++z1) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (int y1 = 0; y1 < BlockSize; ++y1) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					const int stride = (z1 * BlockSize + y1) * BlockSize;
					for (int x1 = 0; x1 < BlockSize; ++x1) {
						// x running fastest
						const int pxIdx = (stride + x1) * 4;
						const uint8_t *v = &rgba[pxIdx];
						if (v[3] == 0u) {
							sampler3.movePositiveX();
							continue;
						}
						const color::RGBA color = flattenRGB(v[0], v[1], v[2], v[3]);
						const uint8_t palIdx = palLookup.findClosestIndex(color);
						voxel::Voxel voxel = voxel::createVoxel(palette, palIdx);
						sampler3.setVoxel(voxel);
						sampler3.movePositiveX();
						empty = false;
					}
					sampler2.movePositiveZ();
				}
				sampler.movePositiveY();
			}
		};
		app::for_parallel(0, BlockSize, fn);
		// TODO: VOXELFORMAT: it looks like the whole node gets the same material in gox...
		// this will remove empty blocks and the final volume might have a smaller region.
		// TODO: VOXELFORMAT: we should remove this once we have sparse volumes support
		if (!empty) {
			voxel::Region destReg(modelVolume->region());
			if (!destReg.containsRegion(blockRegion)) {
				destReg.accumulate(blockRegion);
				voxel::RawVolume *newVolume = new voxel::RawVolume(destReg);
				newVolume->copyInto(*modelVolume);
				delete modelVolume;
				modelVolume = newVolume;
			}
			voxelutil::mergeVolumes(modelVolume, blockVolume, blockRegion, blockRegion);
		}
		delete blockVolume;
	}
	bool visible = true;
	char dictKey[256];
	char dictValue[256];
	int valueLength = 0;
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(core::String::format("model %i", size));
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue, valueLength)) {
		if (!strcmp(dictKey, "name")) {
			// "name" 255 chars max
			node.setName(dictValue);
		} else if (!strcmp(dictKey, "visible")) {
			// "visible" (bool)
			visible = *(const bool *)dictValue;
		} else if (!strcmp(dictKey, "mat")) {
			// "mat" (4x4 matrix)
			scenegraph::SceneGraphTransform transform;
			io::MemoryReadStream subStream(dictValue, sizeof(float) * 16);
			glm::mat4 mat(0.0f);
			// TODO: VOXELFORMAT: axis
			for (int i = 0; i < 16; ++i) {
				subStream.readFloat(mat[i / 4][i % 4]);
			}
			transform.setWorldMatrix(mat);
			node.setTransform(keyFrameIdx, transform);
		} else if (!strcmp(dictKey, "img-path") || !strcmp(dictKey, "id")) {
			// "img-path" model texture path
			// "id" unique id
			node.setProperty(dictKey, dictValue);
		} else if (!strcmp(dictKey, "base_id") || !strcmp(dictKey, "material")) {
			// "base_id" int
			// "material" int (index)
			io::MemoryReadStream subStream(dictValue, sizeof(uint32_t));
			int32_t v;
			subStream.readInt32(v);
			node.setProperty(dictKey, core::string::toString(v));
		} else if (!strcmp(dictKey, "color")) {
			io::MemoryReadStream subStream(dictValue, sizeof(uint32_t));
			uint32_t color;
			subStream.readUInt32(color);
			node.setColor(color::RGBA(color));
		} else if (!strcmp(dictKey, "box") || !strcmp(dictKey, "shape")) {
			// "box" 4x4 bounding box float
			// "shape" - currently unsupported TODO
		} else {
			Log::debug("LAYR chunk with key: %s and size %i", dictKey, valueLength);
		}
	}

	voxel::RawVolume *mirrored = voxelutil::mirrorAxis(modelVolume, math::Axis::X);
	delete modelVolume;
	if (voxel::RawVolume *cropped = voxelutil::cropVolume(mirrored)) {
		delete mirrored;
		const glm::ivec3 mins = cropped->region().getLowerCorner();
		cropped->translate(-mins);

		scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
		transform.setWorldTranslation(mins);

		node.setVolume(cropped, true);
	} else {
		node.setVolume(mirrored, true);
		mirrored = nullptr;
	}
	node.setVisible(visible);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));
	return true;
}

bool GoxFormat::loadChunk_BL16(State &state, const GoxChunk &c, io::SeekableReadStream &stream) {
	uint8_t *png = (uint8_t *)core_malloc(c.length);
	wrapBool(loadChunk_ReadData(stream, (char *)png, c.length))
	image::ImagePtr img = image::createEmptyImage("gox-voxeldata");
	io::MemoryReadStream pngStream(png, c.length);
	bool success = img->load(image::ImageType::PNG, pngStream, pngStream.size());
	core_free(png);
	if (!success) {
		Log::error("Failed to load png chunk");
		return false;
	}
	if (img->width() != 64 || img->height() != 64 || img->components() != 4) {
		Log::error("Invalid image dimensions: %i:%i", img->width(), img->height());
		return false;
	}
	Log::debug("Found BL16 with index %i", (int)state.images.size());
	state.images.push_back(img);
	return true;
}

bool GoxFormat::loadChunk_MATE(State &state, const GoxChunk &c, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	int valueLength = 0;
	core::String name;
	palette::Material material;
	bool emissionFound = false;
	glm::vec4 color(0.0f);
	glm::vec3 emission(0.0f);

	while (loadChunk_DictEntry(c, stream, dictKey, dictValue, valueLength)) {
		if (!strcmp(dictKey, "name")) {
			// 127 chars max
			name = dictValue;
		} else {
			io::MemoryReadStream subStream(dictValue, valueLength);
			if (!strcmp(dictKey, "color")) {
				// "color" 4xfloat
				wrapBool(io::readColor(subStream, color))
			} else if (!strcmp(dictKey, "metallic")) {
				// "metallic" float
				float metallic = 0.0f;
				subStream.readFloat(metallic);
				if (metallic > 0.0f) {
					material.type = palette::MaterialType::Metal;
					material.setValue(palette::MaterialProperty::MaterialMetal, metallic);
				}
			} else if (!strcmp(dictKey, "roughness")) {
				// "roughness" float
				float roughness = 0.0f;
				subStream.readFloat(roughness);
				material.setValue(palette::MaterialProperty::MaterialRoughness, roughness);
			} else if (!strcmp(dictKey, "emission")) {
				// "emission" 3xfloat
				for (int i = 0; i < 3; ++i) {
					subStream.readFloat(emission[i]);
				}
				emissionFound = true;
			} else {
				Log::debug("MATE chunk with key: %s and size %i", dictKey, valueLength);
			}
		}
		if (emissionFound) {
			float emissionFactor = 0.0f;
			for (int i = 0; i < 3; ++i) {
				emissionFactor = core_max(emissionFactor, glm::abs(color[i] - emission[i]));
			}
			material.setValue(palette::MaterialProperty::MaterialEmit, emissionFactor);
		}
	}
	if (name.empty()) {
		return false;
	}
	state.materials.put(name, material);
	return true;
}

bool GoxFormat::loadChunk_CAMR(State &state, const GoxChunk &c, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	int valueLength = 0;
	scenegraph::SceneGraphNodeCamera node;
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue, valueLength)) {
		if (!strcmp(dictKey, "name")) {
			// "name" 127 chars max
			node.setName(dictValue);
		} else if (!strcmp(dictKey, "active")) {
			// "active" no value - active scene camera if this key is available
			node.setProperty(dictKey, "true");
		} else if (!strcmp(dictKey, "dist")) {
			// "dist" float
			io::MemoryReadStream subStream(dictValue, valueLength);
			float farPlane = 0.0f;
			if (subStream.readFloat(farPlane)) {
				node.setFarPlane(farPlane);
			}
		} else if (!strcmp(dictKey, "ortho")) {
			// "ortho" bool
			const bool ortho = *(const bool *)dictValue;
			if (ortho) {
				node.setOrthographic();
			} else {
				node.setPerspective();
			}
		} else if (!strcmp(dictKey, "mat")) {
			// "mat" 4x4 float
			scenegraph::SceneGraphTransform transform;
			io::MemoryReadStream subStream(dictValue, valueLength);
			glm::mat4 mat(0.0f);
			// TODO: VOXELFORMAT: axis
			for (int i = 0; i < 16; ++i) {
				subStream.readFloat(mat[i / 4][i % 4]);
			}
			transform.setWorldMatrix(mat);
			const scenegraph::KeyFrameIndex keyFrameIdx = 0;
			node.setTransform(keyFrameIdx, transform);
		} else {
			Log::debug("CAMR chunk with key: %s and size %i", dictKey, valueLength);
		}
	}
	sceneGraph.emplace(core::move(node));
	return true;
}

bool GoxFormat::loadChunk_IMG(State &state, const GoxChunk &c, io::SeekableReadStream &stream,
							  scenegraph::SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	int valueLength = 0;
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue, valueLength)) {
		Log::debug("IMG chunk with key: %s and size %i", dictKey, valueLength);
		// "box" 4x4 float bounding box
	}
	return true;
}

bool GoxFormat::loadChunk_LIGH(State &state, const GoxChunk &c, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph) {
	char dictKey[256];
	char dictValue[256];
	int valueLength = 0;
	bool fixed = false;
	float intensity = 0.0f;
	float pitch = 0.0f;
	float yaw = 0.0f;
	float ambient = 0.0f;
	float shadow = 0.0f;
	while (loadChunk_DictEntry(c, stream, dictKey, dictValue, valueLength)) {
		io::MemoryReadStream subStream(dictValue, valueLength);
		if (!strcmp(dictKey, "pitch")) {
			subStream.readFloat(pitch);
		} else if (!strcmp(dictKey, "yaw")) {
			subStream.readFloat(yaw);
		} else if (!strcmp(dictKey, "intensity")) {
			subStream.readFloat(intensity);
		} else if (!strcmp(dictKey, "fixed")) {
			fixed = subStream.readBool();
		} else if (!strcmp(dictKey, "ambient")) {
			subStream.readFloat(ambient);
		} else if (!strcmp(dictKey, "shadow")) {
			subStream.readFloat(shadow);
		} else {
			Log::debug("LIGH chunk with key: %s and size %i", dictKey, valueLength);
		}
	}
	Log::debug("Loaded LIGH chunk with pitch: %f, yaw: %f, intensity: %f, fixed: %i, ambient: %f, shadow: %f", pitch,
		yaw, intensity, fixed, ambient, shadow);
	return true;
}

size_t GoxFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return 0;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))

	if (magic != FourCC('G', 'O', 'X', ' ')) {
		Log::error("Invalid gox magic");
		return false;
	}

	State state;
	wrap(stream->readInt32(state.version))

	if (state.version > 2) {
		Log::error("Unknown gox format version found: %u", state.version);
		return false;
	}

	GoxChunk c;
	while (loadChunk_Header(c, *stream)) {
		if (c.type == FourCC('B', 'L', '1', '6')) {
			wrapBool(loadChunk_BL16(state, c, *stream))
		} else {
			stream->seek(c.length, SEEK_CUR);
		}
		loadChunk_ValidateCRC(*stream);
	}

	palette::RGBABuffer colors;
	for (image::ImagePtr &img : state.images) {
		for (int x = 0; x < img->width(); ++x) {
			for (int y = 0; y < img->height(); ++y) {
				const color::RGBA rgba = img->colorAt(x, y);
				if (rgba.a == 0) {
					continue;
				}
				colors.put(flattenRGB(rgba), true);
			}
		}
	}

	return createPalette(colors, palette);
}

bool GoxFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))

	if (magic != FourCC('G', 'O', 'X', ' ')) {
		Log::error("Invalid gox magic");
		return false;
	}

	State state;
	wrap(stream->readInt32(state.version))

	if (state.version > 2) {
		Log::error("Unknown gox format version found: %u", state.version);
		return false;
	}

	GoxChunk c;
	while (loadChunk_Header(c, *stream)) {
		if (c.type == FourCC('B', 'L', '1', '6')) {
			wrapBool(loadChunk_BL16(state, c, *stream))
		} else if (c.type == FourCC('L', 'A', 'Y', 'R')) {
			wrapBool(loadChunk_LAYR(state, c, *stream, sceneGraph, palette))
		} else if (c.type == FourCC('C', 'A', 'M', 'R')) {
			wrapBool(loadChunk_CAMR(state, c, *stream, sceneGraph))
		} else if (c.type == FourCC('M', 'A', 'T', 'E')) {
			wrapBool(loadChunk_MATE(state, c, *stream, sceneGraph))
		} else if (c.type == FourCC('I', 'M', 'G', ' ')) {
			wrapBool(loadChunk_IMG(state, c, *stream, sceneGraph))
		} else if (c.type == FourCC('L', 'I', 'G', 'H')) {
			wrapBool(loadChunk_LIGH(state, c, *stream, sceneGraph))
		} else {
			stream->seek(c.length, SEEK_CUR);
		}
		loadChunk_ValidateCRC(*stream);
	}
	return !sceneGraph.empty();
}

bool GoxFormat::saveChunk_DictEntryHeader(io::WriteStream &stream, const core::String &key, size_t valueSize) {
	const int keyLength = (int)key.size();
	wrapBool(stream.writeUInt32(keyLength))
	if (stream.write(key.c_str(), keyLength) == -1) {
		Log::error("Failed to write dict entry key");
		return false;
	}
	wrapBool(stream.writeUInt32((uint32_t)valueSize))
	// here comes the data
	return true;
}

bool GoxFormat::saveChunk_DictString(io::WriteStream &stream, const core::String &key, const core::String &value) {
	if (!saveChunk_DictEntryHeader(stream, key, value.size())) {
		return false;
	}
	return stream.write(value.c_str(), value.size()) != -1;
}

bool GoxFormat::saveChunk_DictFloat(io::WriteStream &stream, const core::String &key, float value) {
	if (!saveChunk_DictEntryHeader(stream, key, sizeof(value))) {
		return false;
	}
	return stream.writeFloat(value);
}

bool GoxFormat::saveChunk_DictBool(io::WriteStream &stream, const core::String &key, bool value) {
	if (!saveChunk_DictEntryHeader(stream, key, sizeof(value))) {
		return false;
	}
	return stream.writeBool(value);
}

bool GoxFormat::saveChunk_DictInt(io::WriteStream &stream, const core::String &key, int32_t value) {
	if (!saveChunk_DictEntryHeader(stream, key, sizeof(value))) {
		return false;
	}
	return stream.writeInt32(value);
}

bool GoxFormat::saveChunk_DictColor(io::WriteStream &stream, const core::String &key, const color::RGBA &value) {
	if (!saveChunk_DictEntryHeader(stream, key, 4 * sizeof(float))) {
		return false;
	}
	const glm::vec4 &color = color::fromRGBA(value);
	return stream.writeFloat(color.r) && stream.writeFloat(color.g) && stream.writeFloat(color.b) &&
		   stream.writeFloat(color.a);
}

bool GoxFormat::saveChunk_DictMat4(io::WriteStream &stream, const core::String &key, const glm::mat4 &value) {
	if (!saveChunk_DictEntryHeader(stream, key, 16 * sizeof(float))) {
		return false;
	}
	const float *p = glm::value_ptr(value);
	// TODO: VOXELFORMAT: axis
	for (int i = 0; i < 16; ++i) {
		if (!stream.writeFloat(*p++)) {
			return false;
		}
	}
	return true;
}

bool GoxFormat::saveChunk_DictVec3(io::WriteStream &stream, const core::String &key, const glm::vec3 &value) {
	if (!saveChunk_DictEntryHeader(stream, key, sizeof(value))) {
		return false;
	}
	return stream.writeFloat(value.x) && stream.writeFloat(value.y) && stream.writeFloat(value.z);
}

bool GoxFormat::saveChunk_CAMR(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) {
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Camera); iter != sceneGraph.end(); ++iter) {
		GoxScopedChunkWriter scoped(stream, FourCC('C', 'A', 'M', 'R'));
		const scenegraph::SceneGraphNodeCamera &cam = toCameraNode(*iter);
		wrapBool(saveChunk_DictString(stream, "name", cam.name()))
		wrapBool(saveChunk_DictString(stream, "active", "false"))
		const float distance = cam.farPlane();
		wrapBool(saveChunk_DictFloat(stream, "dist", distance))
		const bool ortho = cam.isOrthographic();
		wrapBool(saveChunk_DictBool(stream, "ortho", ortho))
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(cam, 0);
		const glm::mat4 &worldMatrix = transform.worldMatrix();
		wrapBool(saveChunk_DictMat4(stream, "mat", worldMatrix))
	}
	return true;
}

bool GoxFormat::saveChunk_PREV(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream,
							  const SaveContext &savectx) {
	ThumbnailContext ctx;
	ctx.outputSize = glm::ivec2(128);
	const image::ImagePtr &image = createThumbnail(sceneGraph, savectx.thumbnailCreator, ctx);
	if (!image || !image->isLoaded()) {
		return true;
	}
	const int64_t pos = stream.pos();
	GoxScopedChunkWriter scoped(stream, FourCC('P', 'R', 'E', 'V'));
	if (!image->writePNG(stream)) {
		Log::warn("Failed to write preview image");
		return stream.seek(pos) == pos;
	}
	return true;
}

bool GoxFormat::saveChunk_LIGH(io::SeekableWriteStream &stream) {
	return true; // not used
}

bool GoxFormat::saveChunk_MATE(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) {
	GoxScopedChunkWriter scoped(stream, FourCC('M', 'A', 'T', 'E'));
	const palette::Palette &palette = sceneGraph.firstPalette();

	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::String &name = core::String::format("mat%i", i);
		wrapBool(saveChunk_DictString(stream, "name", name))
		const color::RGBA rgba = palette.color(i);
		wrapBool(saveChunk_DictColor(stream, "color", rgba));
		const palette::Material &material = palette.material(i);
		const color::RGBA emitRGBA = palette.emitColor(i);
		const glm::vec3 &emitColor = glm::clamp(color::fromRGBA(emitRGBA) * material.value(palette::MaterialProperty::MaterialEmit), 0.0f, 1.0f);
		wrapBool(saveChunk_DictFloat(stream, "metallic", material.value(palette::MaterialProperty::MaterialMetal)))
		wrapBool(saveChunk_DictFloat(stream, "roughness", material.value(palette::MaterialProperty::MaterialRoughness)))
		wrapBool(saveChunk_DictVec3(stream, "emission", emitColor))
	}
	return true;
}

bool GoxFormat::saveChunk_LAYR(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
							   int numBlocks) {
	int blockUid = 0;
	int layerId = 0;
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		glm::ivec3 mins, maxs;
		calcMinsMaxs(region, glm::ivec3(BlockSize), mins, maxs);

		GoxScopedChunkWriter scoped(stream, FourCC('L', 'A', 'Y', 'R'));
		int layerBlocks = 0;
		auto func = [&](int x, int y, int z, const voxel::Voxel &) {
			if (isEmptyBlock(sceneGraph.resolveVolume(node), glm::ivec3(BlockSize), x, y, z)) {
				return;
			}
			++layerBlocks;
		};
		voxelutil::visitVolume(*sceneGraph.resolveVolume(node), voxel::Region(mins, maxs), BlockSize, BlockSize,
							   BlockSize, func, voxelutil::VisitAll());

		Log::debug("blocks: %i", layerBlocks);

		wrapBool(stream.writeUInt32(layerBlocks))

		for (int y = mins.y; y <= maxs.y; y += BlockSize) {
			for (int z = mins.z; z <= maxs.z; z += BlockSize) {
				for (int x = mins.x; x <= maxs.x; x += BlockSize) {
					if (isEmptyBlock(sceneGraph.resolveVolume(node), glm::ivec3(BlockSize), x, y, z)) {
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
		wrapBool(saveChunk_DictString(stream, "name", node.name()))
		glm::mat4 mat(1.0f);
		wrapBool(saveChunk_DictMat4(stream, "mat", mat))
		wrapBool(saveChunk_DictInt(stream, "id", layerId))
		const color::RGBA layerRGBA = node.color();
		wrapBool(saveChunk_DictColor(stream, "color", layerRGBA.rgba))
#if 0
		wrapBool(saveChunk_DictEntry(stream, "base_id", &layer->base_id))
		wrapBool(saveChunk_DictEntry(stream, "material", &material_idx))
#endif
		wrapBool(saveChunk_DictBool(stream, "visible", node.visible()))

		++layerId;
	}
	if (numBlocks != 0) {
		Log::error("Invalid amount of blocks");
		return false;
	}
	return true;
}

bool GoxFormat::saveChunk_BL16(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph, int &blocks) {
	blocks = 0;
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		glm::ivec3 mins, maxs;
		calcMinsMaxs(region, glm::ivec3(BlockSize), mins, maxs);

		voxel::RawVolume *mirrored = voxelutil::mirrorAxis(sceneGraph.resolveVolume(node), math::Axis::X);
		for (int by = mins.y; by <= maxs.y; by += BlockSize) {
			for (int bz = mins.z; bz <= maxs.z; bz += BlockSize) {
				for (int bx = mins.x; bx <= maxs.x; bx += BlockSize) {
					if (isEmptyBlock(mirrored, glm::ivec3(BlockSize), bx, by, bz)) {
						continue;
					}
					GoxScopedChunkWriter scoped(stream, FourCC('B', 'L', '1', '6'));
					const voxel::Region blockRegion(bx, by, bz, bx + BlockSize - 1, by + BlockSize - 1,
													bz + BlockSize - 1);
					const size_t size = (size_t)BlockSize * BlockSize * BlockSize * 4;
					uint32_t *data = (uint32_t *)core_malloc(size);
					int offset = 0;
					const palette::Palette &palette = node.palette();
					auto func = [&](int, int, int, const voxel::Voxel &voxel) {
						if (voxel::isAir(voxel.getMaterial())) {
							data[offset++] = 0;
						} else {
							data[offset++] = palette.color(voxel.getColor());
						}
					};
					voxelutil::visitVolume(*mirrored, blockRegion, func, voxelutil::VisitAll(),
										   voxelutil::VisitorOrder::YZX);

					image::Image image2("##");
					if (!image2.loadRGBA((const uint8_t*)data, 64, 64)) {
						Log::error("Could not load image data");
						core_free(data);
						delete mirrored;
						return false;
					}
					core_free(data);
					if (!image2.writePNG(stream)) {
						Log::error("Could not write png into gox stream");
						delete mirrored;
						return false;
					}
					Log::debug("Saved BL16 chunk %i", blocks);
					++blocks;
				}
			}
		}
		delete mirrored;
	}
	return true;
}

bool GoxFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	wrapSave(stream->writeUInt32(FourCC('G', 'O', 'X', ' ')))
	wrapSave(stream->writeUInt32(2))

	wrapBool(saveChunk_PREV(sceneGraph, *stream, ctx))
	int blocks = 0;
	wrapBool(saveChunk_BL16(*stream, sceneGraph, blocks))
	wrapBool(saveChunk_MATE(*stream, sceneGraph))
	wrapBool(saveChunk_LAYR(*stream, sceneGraph, blocks))
	wrapBool(saveChunk_CAMR(*stream, sceneGraph))
	wrapBool(saveChunk_LIGH(*stream))

	return true;
}

#undef wrapBool
#undef wrapImg
#undef wrap
#undef wrapSave

} // namespace voxelformat
