/**
 * @file
 */

#include "VCdxFormat.h"
#include "color/RGBA.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "image/ImageType.h"
#include "io/Archive.h"
#include "io/BrotliReadStream.h"
#include "io/BrotliWriteStream.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/FormatThumbnail.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vcdx file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",           \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Could not load vcdx file: Failure at " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__);           \
		return false;                                                                                                  \
	}

#define wrapImg(read)                                                                                                  \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vcdx screenshot: Not enough data in stream " CORE_STRINGIFY(read));                  \
		return image::ImagePtr();                                                                                      \
	}

#define wrapSave(write)                                                                                                \
	if ((write) == false) {                                                                                            \
		Log::error("Could not save vcdx file: " CORE_STRINGIFY(write) " failed (line %i)", (int)__LINE__);              \
		return false;                                                                                                  \
	}

namespace priv {

static const int Version = 10;
static const int ChunkSize = 32;
static const int ChunkVoxels = 32768;
static const int MaxPaletteSlots = 1024;
static const int ChunkBytes = ChunkVoxels * 2;

static palette::MaterialType fromVcdxMaterialType(uint8_t type) {
	switch (type) {
	case 1:
		return palette::MaterialType::Metal;
	case 2:
		return palette::MaterialType::Emit;
	case 3:
		return palette::MaterialType::Glass;
	default:
		return palette::MaterialType::Diffuse;
	}
}

static uint8_t toVcdxMaterialType(palette::MaterialType type) {
	switch (type) {
	case palette::MaterialType::Metal:
		return 1;
	case palette::MaterialType::Emit:
		return 2;
	case palette::MaterialType::Glass:
		return 3;
	default:
		return 0;
	}
}

static bool skipBytes(io::SeekableReadStream &stream, int64_t n) {
	if (n <= 0) {
		return true;
	}
	return stream.skip(n) != -1;
}

static bool readVec3i(io::ReadStream &stream, glm::ivec3 &v) {
	return stream.readInt32(v.x) == 0 && stream.readInt32(v.y) == 0 && stream.readInt32(v.z) == 0;
}

static bool writeVec3i(io::WriteStream &stream, const glm::ivec3 &v) {
	return stream.writeInt32(v.x) && stream.writeInt32(v.y) && stream.writeInt32(v.z);
}

static bool writeVec3f(io::WriteStream &stream, float x, float y, float z) {
	return stream.writeFloat(x) && stream.writeFloat(y) && stream.writeFloat(z);
}

static bool skipVec3f(io::ReadStream &stream) {
	float dummy;
	return stream.readFloat(dummy) == 0 && stream.readFloat(dummy) == 0 && stream.readFloat(dummy) == 0;
}

static glm::ivec3 chunkCoord(int x, int y, int z) {
	return glm::ivec3(x >> 5, y >> 5, z >> 5);
}

static int chunkIndex(int x, int y, int z) {
	const int lx = x & 31;
	const int ly = y & 31;
	const int lz = z & 31;
	return lx | (ly << 5) | (lz << 10);
}

static palette::Material makeMaterial(uint8_t type, float roughness, float metallic, float emission, float ior) {
	palette::Material mat;
	mat.type = fromVcdxMaterialType(type);
	mat.setValue(palette::MaterialProperty::MaterialRoughness, roughness);
	mat.setValue(palette::MaterialProperty::MaterialMetal, metallic);
	mat.setValue(palette::MaterialProperty::MaterialEmit, emission);
	mat.setValue(palette::MaterialProperty::MaterialIndexOfRefraction, ior);
	return mat;
}

static bool readSlot(io::ReadStream &stream, color::RGBA &color, palette::Material &material) {
	if (stream.readUInt8(color.r) != 0 || stream.readUInt8(color.g) != 0 || stream.readUInt8(color.b) != 0 ||
		stream.readUInt8(color.a) != 0) {
		return false;
	}
	uint8_t type;
	if (stream.readUInt8(type) != 0) {
		return false;
	}
	float roughness, metallic, emission, ior, transparency;
	if (stream.readFloat(roughness) != 0 || stream.readFloat(metallic) != 0 || stream.readFloat(emission) != 0 ||
		stream.readFloat(ior) != 0 || stream.readFloat(transparency) != 0) {
		return false;
	}
	(void)transparency;
	material = makeMaterial(type, roughness, metallic, emission, ior);
	return true;
}

static bool writeSlot(io::WriteStream &stream, color::RGBA color, uint8_t type, float roughness, float metallic,
					  float emission, float ior, float transparency) {
	return stream.writeUInt8(color.r) && stream.writeUInt8(color.g) && stream.writeUInt8(color.b) &&
		   stream.writeUInt8(color.a) && stream.writeUInt8(type) && stream.writeFloat(roughness) &&
		   stream.writeFloat(metallic) && stream.writeFloat(emission) && stream.writeFloat(ior) &&
		   stream.writeFloat(transparency);
}

static bool writeDefaultSlot(io::WriteStream &stream, color::RGBA color) {
	return writeSlot(stream, color, 0, 0.8f, 1.0f, 0.0f, 1.5f, 0.9f);
}

static bool skipPaletteRows(io::SeekableReadStream &stream, int paletteSize) {
	const int rows = core_min(paletteSize / 16, 64);
	for (int i = 0; i < rows; ++i) {
		core::String label;
		if (!stream.readDotNetString(label)) {
			return false;
		}
		(void)stream.readBool();
	}
	return true;
}

static bool readPaletteRows(io::SeekableReadStream &stream, int paletteSize, palette::Palette &palette) {
	const int rows = core_min(paletteSize / 16, 64);
	for (int i = 0; i < rows; ++i) {
		core::String label;
		if (!stream.readDotNetString(label)) {
			return false;
		}
		(void)stream.readBool();
		const int slot = i * 16;
		if (slot < palette::PaletteMaxColors && !label.empty()) {
			palette.setColorName((uint8_t)slot, label);
		}
	}
	return true;
}

static bool writePaletteRows(io::WriteStream &stream, const palette::Palette &palette) {
	for (int i = 0; i < 64; ++i) {
		core::String label;
		const int slot = i * 16;
		if (slot < palette.colorCount()) {
			label = palette.colorName((uint8_t)slot);
		}
		if (!stream.writeDotNetString(label)) {
			return false;
		}
		if (!stream.writeBool(false)) {
			return false;
		}
	}
	return true;
}

static bool writePaletteData(io::WriteStream &stream, const palette::Palette &palette) {
	if (!writeDefaultSlot(stream, color::RGBA(0, 0, 0, 0))) {
		return false;
	}
	for (int i = 1; i < MaxPaletteSlots; ++i) {
		if (i < palette.colorCount()) {
			const palette::Material &mat = palette.material((uint8_t)i);
			const float roughness =
				mat.has(palette::MaterialProperty::MaterialRoughness) ? mat.roughness : 0.8f;
			const float metallic = mat.has(palette::MaterialProperty::MaterialMetal) ? mat.metal : 1.0f;
			const float emission = mat.has(palette::MaterialProperty::MaterialEmit) ? mat.emit : 0.0f;
			const float ior =
				mat.has(palette::MaterialProperty::MaterialIndexOfRefraction) ? mat.indexOfRefraction : 1.5f;
			if (!writeSlot(stream, palette.color((uint8_t)i), toVcdxMaterialType(mat.type), roughness, metallic,
						   emission, ior, 0.9f)) {
				return false;
			}
		} else {
			if (!writeDefaultSlot(stream, color::RGBA(0, 0, 0, 0))) {
				return false;
			}
		}
	}
	return writePaletteRows(stream, palette);
}

static uint8_t mapIndex(uint16_t idx, const palette::Palette &palette, const color::RGBA *allColors) {
	if (idx == 0) {
		return 0;
	}
	if (idx >= MaxPaletteSlots) {
		idx = MaxPaletteSlots - 1;
	}
	if (idx < (uint16_t)palette.colorCount() && idx < (uint16_t)palette::PaletteMaxColors) {
		return (uint8_t)idx;
	}
	const int match = palette.getClosestMatch(allColors[idx], 0);
	if (match == palette::PaletteColorNotFound || match == 0) {
		return 1;
	}
	return (uint8_t)match;
}

static bool skipChunks(io::SeekableReadStream &stream, int32_t count) {
	for (int32_t i = 0; i < count; ++i) {
		if (!skipBytes(stream, 12 + ChunkBytes)) {
			return false;
		}
	}
	return true;
}

static bool skipRig(io::SeekableReadStream &stream, int version, int layerCount) {
	int32_t boneCount;
	if (stream.readInt32(boneCount) != 0) {
		return false;
	}
	if (boneCount < 0) {
		return false;
	}
	for (int32_t i = 0; i < boneCount; ++i) {
		core::String dummy;
		if (!stream.readDotNetString(dummy) || !stream.readDotNetString(dummy)) {
			return false;
		}
		if (!skipVec3f(stream) || !skipVec3f(stream)) {
			return false;
		}
		if (version >= 6) {
			int32_t id;
			if (stream.readInt32(id) != 0) {
				return false;
			}
		}
	}
	for (int i = 0; i < layerCount; ++i) {
		core::String dummy;
		if (!stream.readDotNetString(dummy)) {
			return false;
		}
		if (version >= 6) {
			int32_t mapChunks;
			if (stream.readInt32(mapChunks) != 0) {
				return false;
			}
			if (!skipChunks(stream, mapChunks)) {
				return false;
			}
		}
	}
	int32_t clipCount;
	if (stream.readInt32(clipCount) != 0) {
		return false;
	}
	if (clipCount < 0) {
		return false;
	}
	for (int32_t c = 0; c < clipCount; ++c) {
		core::String dummy;
		if (!stream.readDotNetString(dummy)) {
			return false;
		}
		float fps;
		int32_t length;
		if (stream.readFloat(fps) != 0 || stream.readInt32(length) != 0) {
			return false;
		}
		(void)stream.readBool();
		int32_t trackCount;
		if (stream.readInt32(trackCount) != 0) {
			return false;
		}
		for (int32_t t = 0; t < trackCount; ++t) {
			if (!stream.readDotNetString(dummy)) {
				return false;
			}
			int32_t keyCount;
			if (stream.readInt32(keyCount) != 0) {
				return false;
			}
			for (int32_t k = 0; k < keyCount; ++k) {
				int32_t frame;
				if (stream.readInt32(frame) != 0) {
					return false;
				}
				float qx, qy, qz, qw;
				if (stream.readFloat(qx) != 0 || stream.readFloat(qy) != 0 || stream.readFloat(qz) != 0 ||
					stream.readFloat(qw) != 0) {
					return false;
				}
				if (!skipVec3f(stream)) {
					return false;
				}
				uint8_t interpolation;
				if (stream.readUInt8(interpolation) != 0) {
					return false;
				}
			}
		}
	}
	if (version < 7) {
		return true;
	}
	uint8_t shape;
	if (stream.readUInt8(shape) != 0) {
		return false;
	}
	core::String dummy;
	if (!stream.readDotNetString(dummy)) {
		return false;
	}
	for (int32_t i = 0; i < boneCount; ++i) {
		if (stream.readBool()) {
			if (!skipBytes(stream, 3)) {
				return false;
			}
		}
	}
	for (int i = 0; i < layerCount; ++i) {
		(void)stream.readBool();
	}
	if (version < 8) {
		return true;
	}
	for (int i = 0; i < layerCount; ++i) {
		if (!skipVec3f(stream) || !skipVec3f(stream) || !skipVec3f(stream)) {
			return false;
		}
		float scale;
		if (stream.readFloat(scale) != 0) {
			return false;
		}
	}
	if (version < 9) {
		return true;
	}
	(void)stream.readBool();
	if (!stream.readDotNetString(dummy)) {
		return false;
	}
	for (int32_t c = 0; c < clipCount; ++c) {
		if (!stream.readDotNetString(dummy)) {
			return false;
		}
		if (version < 10) {
			(void)stream.readBool();
			continue;
		}
		(void)stream.readBool();
		int32_t n;
		if (stream.readInt32(n) != 0) {
			return false;
		}
		for (int32_t i = 0; i < n; ++i) {
			if (!stream.readDotNetString(dummy)) {
				return false;
			}
		}
	}
	return true;
}

static bool writeEmptyRig(io::WriteStream &stream, int layerCount) {
	if (!stream.writeInt32(0)) {
		return false;
	}
	for (int i = 0; i < layerCount; ++i) {
		if (!stream.writeDotNetString("") || !stream.writeInt32(0)) {
			return false;
		}
	}
	if (!stream.writeInt32(0) || !stream.writeUInt8(0) || !stream.writeDotNetString("Armature")) {
		return false;
	}
	for (int i = 0; i < layerCount; ++i) {
		if (!stream.writeBool(false)) {
			return false;
		}
	}
	for (int i = 0; i < layerCount; ++i) {
		if (!writeVec3f(stream, 0.0f, 0.0f, 0.0f) || !writeVec3f(stream, 0.0f, 0.0f, 0.0f) ||
			!writeVec3f(stream, 0.0f, 0.0f, 0.0f) || !stream.writeFloat(1.0f)) {
			return false;
		}
	}
	return stream.writeBool(false) && stream.writeDotNetString("");
}

static void expandWorldAabb(glm::ivec3 &mins, glm::ivec3 &maxs, bool &haveBounds, const voxel::Region &region,
							const glm::mat4 &worldMat) {
	const glm::ivec3 rmin = region.getLowerCorner();
	const glm::ivec3 rmax = region.getUpperCorner();
	for (int i = 0; i < 8; ++i) {
		const glm::vec3 corner((i & 1) ? (float)rmax.x : (float)rmin.x, (i & 2) ? (float)rmax.y : (float)rmin.y,
							   (i & 4) ? (float)rmax.z : (float)rmin.z);
		const glm::ivec3 world(glm::round(glm::vec3(worldMat * glm::vec4(corner, 1.0f))));
		if (!haveBounds) {
			mins = maxs = world;
			haveBounds = true;
		} else {
			mins = glm::min(mins, world);
			maxs = glm::max(maxs, world);
		}
	}
}

static bool writeLayerVolume(io::WriteStream &stream, const voxel::RawVolume *volume) {
	core::DynamicArray<glm::ivec3> coords;
	const voxel::Region region = volume != nullptr ? volume->region() : voxel::Region(0, 0);
	if (volume != nullptr) {
		voxelutil::visitVolume(*volume, [&](int x, int y, int z, const voxel::Voxel &) {
			const glm::ivec3 c = chunkCoord(x, y, z);
			for (size_t i = 0; i < coords.size(); ++i) {
				if (coords[i] == c) {
					return;
				}
			}
			coords.push_back(c);
		});
		coords.sort([](const glm::ivec3 &a, const glm::ivec3 &b) {
			if (a.z != b.z) {
				return a.z > b.z;
			}
			if (a.y != b.y) {
				return a.y > b.y;
			}
			return a.x > b.x;
		});
	}
	if (!stream.writeInt32((int32_t)coords.size())) {
		return false;
	}
	for (const glm::ivec3 &coord : coords) {
		uint16_t voxels[ChunkVoxels];
		core_memset(voxels, 0, sizeof(voxels));
		const int x0 = coord.x * ChunkSize;
		const int y0 = coord.y * ChunkSize;
		const int z0 = coord.z * ChunkSize;
		for (int lz = 0; lz < ChunkSize; ++lz) {
			for (int ly = 0; ly < ChunkSize; ++ly) {
				for (int lx = 0; lx < ChunkSize; ++lx) {
					const int x = x0 + lx;
					const int y = y0 + ly;
					const int z = z0 + lz;
					if (!region.containsPoint(x, y, z)) {
						continue;
					}
					const voxel::Voxel &voxel = volume->voxel(x, y, z);
					if (voxel::isAir(voxel.getMaterial())) {
						continue;
					}
					voxels[chunkIndex(x, y, z)] = voxel.getColor();
				}
			}
		}
		if (!writeVec3i(stream, coord)) {
			return false;
		}
		for (int i = 0; i < ChunkVoxels; ++i) {
			if (!stream.writeUInt16(voxels[i])) {
				return false;
			}
		}
	}
	return true;
}

static bool writeInfoJson(io::WriteStream &stream, int models, int64_t voxels, const glm::ivec3 &size,
						  const core::DynamicArray<core::String> &names) {
	io::BufferedReadWriteStream json(512);
	if (!json.writeString("{\"models\":", false) || !json.writeString(core::string::toString(models), false) ||
		!json.writeString(",\"voxels\":", false) || !json.writeString(core::string::toString(voxels), false) ||
		!json.writeString(",\"size\":[", false) || !json.writeString(core::string::toString(size.x), false) ||
		!json.writeString(",", false) || !json.writeString(core::string::toString(size.y), false) ||
		!json.writeString(",", false) || !json.writeString(core::string::toString(size.z), false) ||
		!json.writeString("],\"names\":[", false)) {
		return false;
	}
	for (size_t i = 0; i < names.size(); ++i) {
		if (i > 0 && !json.writeString(",", false)) {
			return false;
		}
		if (!json.writeJsonString(names[i])) {
			return false;
		}
	}
	if (!json.writeString("]}", false)) {
		return false;
	}
	core::String payload;
	if (json.seek(0) == -1 || !json.readString((int)json.size(), payload, false)) {
		return false;
	}
	return stream.writeDotNetString(payload);
}

static bool skipHeaderPreview(io::SeekableReadStream &stream, int32_t version) {
	if (version < 4) {
		return true;
	}
	core::String info;
	if (!stream.readDotNetString(info)) {
		return false;
	}
	int32_t thumbLen;
	if (stream.readInt32(thumbLen) != 0) {
		return false;
	}
	if (thumbLen < 0) {
		return false;
	}
	return skipBytes(stream, thumbLen);
}

static bool fillPalette(palette::Palette &palette, color::RGBA *allColors, palette::Material *allMats, int paletteSize) {
	palette.setSize(palette::PaletteMaxColors);
	palette.setColor(0, color::RGBA(0, 0, 0, 0));
	palette.setMaterial(0, palette::Material());
	int last = 0;
	const int copy = core_min(paletteSize, palette::PaletteMaxColors);
	for (int i = 1; i < copy; ++i) {
		palette.setColor((uint8_t)i, allColors[i]);
		palette.setMaterial((uint8_t)i, allMats[i]);
		if (allColors[i].a > 0) {
			last = i;
		}
	}
	palette.setSize(core_max(last + 1, 1));
	return palette.colorCount() > 0;
}

} // namespace priv

int VCdxFormat::emptyPaletteIndex() const {
	return 0;
}

static bool readSlots(io::SeekableReadStream &stream, int paletteSize, color::RGBA *allColors,
					  palette::Material *allMats) {
	for (int i = 0; i < paletteSize; ++i) {
		if (!priv::readSlot(stream, allColors[i], allMats[i])) {
			return false;
		}
	}
	return true;
}

size_t VCdxFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return 0;
	}
	uint32_t magic;
	if (stream->readUInt32(magic) != 0) {
		return 0;
	}
	if (magic != FourCC('V', 'C', 'D', 'X')) {
		Log::error("Invalid vcdx magic");
		return 0;
	}
	int32_t version;
	if (stream->readInt32(version) != 0) {
		return 0;
	}
	if (version < 1 || version > priv::Version) {
		Log::error("Unsupported vcdx version %i", version);
		return 0;
	}
	if (!priv::skipHeaderPreview(*stream, version)) {
		return 0;
	}
	io::BrotliReadStream body(*stream);
	if (body.size() <= 0) {
		Log::error("Failed to decompress vcdx body");
		return 0;
	}
	(void)ctx;
	color::RGBA allColors[priv::MaxPaletteSlots];
	palette::Material allMats[priv::MaxPaletteSlots];
	int paletteSize = 256;
	if (version >= 4) {
		if (body.readInt32(paletteSize) != 0) {
			return 0;
		}
	}
	if (paletteSize <= 0 || paletteSize > priv::MaxPaletteSlots) {
		Log::error("Invalid vcdx palette size %i", paletteSize);
		return 0;
	}
	if (!readSlots(body, paletteSize, allColors, allMats)) {
		return 0;
	}
	if (version >= 4) {
		if (!priv::readPaletteRows(body, paletteSize, palette)) {
			return 0;
		}
	} else {
		core::String renderJson;
		if (!body.readDotNetString(renderJson)) {
			return 0;
		}
		if (version >= 2) {
			if (body.readBool()) {
				if (!priv::skipBytes(body, 24)) {
					return 0;
				}
			}
			if (!priv::readPaletteRows(body, 256, palette)) {
				return 0;
			}
		}
	}
	if (!priv::fillPalette(palette, allColors, allMats, paletteSize)) {
		return 0;
	}
	return palette.colorCount();
}

image::ImagePtr VCdxFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return image::ImagePtr();
	}
	uint32_t magic;
	wrapImg(stream->readUInt32(magic))
	if (magic != FourCC('V', 'C', 'D', 'X')) {
		Log::error("Invalid vcdx magic");
		return image::ImagePtr();
	}
	int32_t version;
	wrapImg(stream->readInt32(version))
	if (version < 4) {
		return image::ImagePtr();
	}
	core::String info;
	if (!stream->readDotNetString(info)) {
		return image::ImagePtr();
	}
	int32_t thumbLen;
	wrapImg(stream->readInt32(thumbLen))
	if (thumbLen <= 0) {
		return image::ImagePtr();
	}
	(void)ctx;
	image::ImagePtr img = image::createEmptyImage(core::string::extractFilename(filename) + ".png");
	if (!img->load(image::ImageType::PNG, *stream, thumbLen)) {
		Log::error("Failed to decode vcdx thumbnail");
		return image::ImagePtr();
	}
	return img;
}

static bool loadLayerVoxels(io::SeekableReadStream &stream, int32_t chunkCount, bool eightBit,
							core::DynamicArray<glm::ivec3> &positions, core::DynamicArray<uint16_t> &indices,
							glm::ivec3 &mins, glm::ivec3 &maxs, bool &hasVoxel) {
	uint8_t raw[priv::ChunkBytes];
	for (int32_t c = 0; c < chunkCount; ++c) {
		glm::ivec3 coord;
		if (!priv::readVec3i(stream, coord)) {
			return false;
		}
		const int bytes = eightBit ? priv::ChunkVoxels : priv::ChunkBytes;
		if (stream.read(raw, bytes) != bytes) {
			Log::error("Failed to read vcdx chunk voxels");
			return false;
		}
		for (int i = 0; i < priv::ChunkVoxels; ++i) {
			uint16_t idx = eightBit ? raw[i] : (uint16_t)(raw[i * 2] | ((uint16_t)raw[i * 2 + 1] << 8));
			if (idx == 0) {
				continue;
			}
			if (idx >= priv::MaxPaletteSlots) {
				idx = priv::MaxPaletteSlots - 1;
			}
			const int lx = i & 31;
			const int ly = (i >> 5) & 31;
			const int lz = (i >> 10) & 31;
			const glm::ivec3 pos(coord.x * priv::ChunkSize + lx, coord.y * priv::ChunkSize + ly,
								 coord.z * priv::ChunkSize + lz);
			if (!hasVoxel) {
				mins = maxs = pos;
				hasVoxel = true;
			} else {
				mins = glm::min(mins, pos);
				maxs = glm::max(maxs, pos);
			}
			positions.push_back(pos);
			indices.push_back(idx);
		}
	}
	return true;
}

bool VCdxFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	if (magic != FourCC('V', 'C', 'D', 'X')) {
		Log::error("Invalid vcdx magic");
		return false;
	}
	int32_t version;
	wrap(stream->readInt32(version))
	if (version < 1 || version > priv::Version) {
		Log::error("Unsupported vcdx version %i", version);
		return false;
	}
	wrapBool(priv::skipHeaderPreview(*stream, version))
	io::BrotliReadStream body(*stream);
	if (body.size() <= 0) {
		Log::error("Failed to decompress vcdx body");
		return false;
	}

	color::RGBA allColors[priv::MaxPaletteSlots];
	palette::Material allMats[priv::MaxPaletteSlots];
	int paletteSize = 256;
	if (version >= 4) {
		wrap(body.readInt32(paletteSize))
	}
	if (paletteSize <= 0 || paletteSize > priv::MaxPaletteSlots) {
		Log::error("Invalid vcdx palette size %i", paletteSize);
		return false;
	}
	for (int i = 0; i < paletteSize; ++i) {
		wrapBool(priv::readSlot(body, allColors[i], allMats[i]))
	}

	if (version >= 4) {
		wrapBool(priv::readPaletteRows(body, paletteSize, palette))
		core::String renderJson;
		wrapBool(body.readDotNetString(renderJson))
		int32_t pageCount;
		int32_t activePalette;
		wrap(body.readInt32(pageCount))
		wrap(body.readInt32(activePalette))
		for (int32_t p = 0; p < pageCount; ++p) {
			core::String pageName;
			wrapBool(body.readDotNetString(pageName))
			for (int i = 0; i < paletteSize; ++i) {
				color::RGBA dummyColor;
				palette::Material dummyMat;
				wrapBool(priv::readSlot(body, dummyColor, dummyMat))
			}
			wrapBool(priv::skipPaletteRows(body, paletteSize))
		}
	} else {
		core::String renderJson;
		wrapBool(body.readDotNetString(renderJson))
		if (version >= 2) {
			uint8_t hasCanvas;
			wrap(body.readUInt8(hasCanvas))
			if (hasCanvas != 0) {
				wrapBool(priv::skipBytes(body, 24))
			}
			wrapBool(priv::readPaletteRows(body, 256, palette))
		}
		if (version >= 3) {
			int32_t pageCount;
			int32_t activePalette;
			wrap(body.readInt32(pageCount))
			wrap(body.readInt32(activePalette))
			for (int32_t p = 0; p < pageCount; ++p) {
				core::String pageName;
				wrapBool(body.readDotNetString(pageName))
				for (int i = 0; i < 256; ++i) {
					color::RGBA dummyColor;
					palette::Material dummyMat;
					wrapBool(priv::readSlot(body, dummyColor, dummyMat))
				}
				wrapBool(priv::skipPaletteRows(body, 256))
			}
		}
	}

	wrapBool(priv::fillPalette(palette, allColors, allMats, paletteSize))

	auto addLayerNode = [&](const core::String &name, bool visible, bool locked, const core::DynamicArray<glm::ivec3> &positions,
							const core::DynamicArray<uint16_t> &indices, bool hasVoxel, const glm::ivec3 &mins,
							const glm::ivec3 &maxs, const glm::ivec3 *canvasMin, const glm::ivec3 *canvasSize,
							int parent) -> bool {
		glm::ivec3 rmin(0);
		glm::ivec3 rmax(0);
		if (hasVoxel) {
			rmin = mins;
			rmax = maxs;
		} else if (canvasMin != nullptr && canvasSize != nullptr) {
			rmin = *canvasMin;
			rmax = *canvasMin + *canvasSize - glm::ivec3(1);
		}
		const voxel::Region region(rmin, rmax);
		if (!checkValidRegion(region)) {
			return false;
		}
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (size_t i = 0; i < positions.size(); ++i) {
			const uint8_t palIdx = priv::mapIndex(indices[i], palette, allColors);
			volume->setVoxel(positions[i], voxel::createVoxel(palette, palIdx));
		}
		cropOnLoad(volume);
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName(name);
		node.setVisible(visible);
		node.setLocked(locked);
		node.setVolume(volume);
		node.setPalette(palette);
		return sceneGraph.emplace(core::move(node), parent) != InvalidNodeId;
	};

	if (version < 4) {
		int32_t layerCount;
		int32_t activeLayer;
		wrap(body.readInt32(layerCount))
		wrap(body.readInt32(activeLayer))
		if (layerCount <= 0) {
			layerCount = 1;
		}
		ctx.report("layers", 0, layerCount);
		const int parent = sceneGraph.root().id();
		int loaded = 0;
		for (int32_t l = 0; l < layerCount; ++l) {
			core::String layerName;
			wrapBool(body.readDotNetString(layerName))
			const bool visible = body.readBool();
			const bool locked = body.readBool();
			int32_t chunkCount;
			wrap(body.readInt32(chunkCount))
			core::DynamicArray<glm::ivec3> positions;
			core::DynamicArray<uint16_t> indices;
			glm::ivec3 mins(0);
			glm::ivec3 maxs(0);
			bool hasVoxel = false;
			wrapBool(loadLayerVoxels(body, chunkCount, true, positions, indices, mins, maxs, hasVoxel))
			if (layerName.empty()) {
				layerName = "Layer 1";
			}
			wrapBool(addLayerNode(layerName, visible, locked, positions, indices, hasVoxel, mins, maxs, nullptr,
								  nullptr, parent))
			++loaded;
			ctx.report("layers", loaded, layerCount);
		}
		return loaded > 0;
	}

	int32_t modelCount;
	int32_t activeModel;
	wrap(body.readInt32(modelCount))
	wrap(body.readInt32(activeModel))
	if (modelCount <= 0) {
		Log::warn("vcdx file has no models");
		return true;
	}
	ctx.report("models", 0, modelCount);
	for (int32_t m = 0; m < modelCount; ++m) {
		core::String modelName;
		wrapBool(body.readDotNetString(modelName))
		const bool hasCanvas = body.readBool();
		glm::ivec3 canvasMin(0);
		glm::ivec3 canvasSize(0);
		const glm::ivec3 *canvasMinPtr = nullptr;
		const glm::ivec3 *canvasSizePtr = nullptr;
		if (hasCanvas) {
			wrapBool(priv::readVec3i(body, canvasMin))
			wrapBool(priv::readVec3i(body, canvasSize))
			canvasMinPtr = &canvasMin;
			canvasSizePtr = &canvasSize;
		}
		int32_t layerCount;
		int32_t activeLayer;
		wrap(body.readInt32(layerCount))
		wrap(body.readInt32(activeLayer))
		if (layerCount <= 0) {
			layerCount = 1;
		}
		int parent = sceneGraph.root().id();
		if (layerCount > 1) {
			scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
			group.setName(modelName.empty() ? "Model 1" : modelName);
			parent = sceneGraph.emplace(core::move(group));
			if (parent == InvalidNodeId) {
				Log::error("Failed to add vcdx model group");
				return false;
			}
		}
		for (int32_t l = 0; l < layerCount; ++l) {
			core::String layerName;
			wrapBool(body.readDotNetString(layerName))
			const bool visible = body.readBool();
			const bool locked = body.readBool();
			int32_t chunkCount;
			wrap(body.readInt32(chunkCount))
			core::DynamicArray<glm::ivec3> positions;
			core::DynamicArray<uint16_t> indices;
			glm::ivec3 mins(0);
			glm::ivec3 maxs(0);
			bool hasVoxel = false;
			wrapBool(loadLayerVoxels(body, chunkCount, false, positions, indices, mins, maxs, hasVoxel))
			core::String nodeName = layerName;
			if (layerCount == 1) {
				nodeName = modelName.empty() ? layerName : modelName;
			}
			if (nodeName.empty()) {
				nodeName = "Layer 1";
			}
			wrapBool(addLayerNode(nodeName, visible, locked, positions, indices, hasVoxel, mins, maxs, canvasMinPtr,
								  canvasSizePtr, parent))
		}
		if (version >= 5) {
			wrapBool(priv::skipRig(body, version, layerCount))
		}
		ctx.report("models", m + 1, modelCount);
	}
	return true;
}

bool VCdxFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	core::DynamicArray<const scenegraph::SceneGraphNode *> models;
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		models.push_back(&(*iter));
	}
	if (models.empty()) {
		Log::error("No models to save to vcdx");
		return false;
	}

	const palette::Palette &palette = sceneGraph.firstPalette();
	int64_t totalVoxels = 0;
	glm::ivec3 size(0);
	glm::ivec3 mins(0);
	glm::ivec3 maxs(0);
	bool haveBounds = false;
	core::String modelName = core::string::extractFilename(filename);
	if (models.size() == 1 && !models[0]->name().empty()) {
		modelName = models[0]->name();
	} else if (modelName.empty()) {
		modelName = "Model 1";
	}
	core::DynamicArray<core::String> names;
	names.push_back(modelName);
	for (const scenegraph::SceneGraphNode *node : models) {
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(*node);
		if (volume == nullptr) {
			continue;
		}
		totalVoxels += voxelutil::countVoxels(*volume);
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(*node, 0);
		priv::expandWorldAabb(mins, maxs, haveBounds, volume->region(), transform.worldMatrix());
	}
	if (haveBounds) {
		size = maxs - mins + glm::ivec3(1);
	}

	wrapSave(stream->writeUInt32(FourCC('V', 'C', 'D', 'X')))
	wrapSave(stream->writeInt32(priv::Version))
	wrapSave(priv::writeInfoJson(*stream, 1, totalVoxels, size, names))

	ThumbnailContext thumbCtx;
	thumbCtx.outputSize = glm::ivec2(192, 192);
	const image::ImagePtr &image = createThumbnail(sceneGraph, ctx.thumbnailCreator, thumbCtx);
	if (image && image->isLoaded()) {
		io::BufferedReadWriteStream png(192 * 192 * 4);
		if (image->writePNG(png) && png.seek(0) != -1) {
			wrapSave(stream->writeInt32((int32_t)png.size()))
			if (stream->write(png.getBuffer(), png.size()) != (int)png.size()) {
				Log::error("Failed to write vcdx thumbnail");
				return false;
			}
		} else {
			wrapSave(stream->writeInt32(0))
		}
	} else {
		wrapSave(stream->writeInt32(0))
	}

	io::BrotliWriteStream brotli(*stream, 4);
	wrapSave(brotli.writeInt32(priv::MaxPaletteSlots))
	wrapSave(priv::writePaletteData(brotli, palette))
	wrapSave(brotli.writeDotNetString("{}"))
	wrapSave(brotli.writeInt32(1))
	wrapSave(brotli.writeInt32(0))
	wrapSave(brotli.writeDotNetString("Default"))
	wrapSave(priv::writePaletteData(brotli, palette))
	wrapSave(brotli.writeInt32(1))
	wrapSave(brotli.writeInt32(0))
	wrapSave(brotli.writeDotNetString(modelName))
	wrapSave(brotli.writeBool(haveBounds))
	if (haveBounds) {
		wrapSave(priv::writeVec3i(brotli, mins))
		wrapSave(priv::writeVec3i(brotli, size))
	}
	wrapSave(brotli.writeInt32((int32_t)models.size()))
	wrapSave(brotli.writeInt32(0))

	int layerIdx = 0;
	for (const scenegraph::SceneGraphNode *node : models) {
		core::String layerName = node->name();
		if (layerName.empty()) {
			layerName = models.size() == 1 ? "Model 1" : core::String::format("Layer %i", layerIdx + 1);
		}
		wrapSave(brotli.writeDotNetString(layerName))
		wrapSave(brotli.writeBool(node->visible()))
		wrapSave(brotli.writeBool(node->locked()))

		const voxel::RawVolume *volume = sceneGraph.resolveVolume(*node);
		core::ScopedPtr<voxel::RawVolume> baked;
		if (volume != nullptr) {
			const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(*node, 0);
			if (!transform.isIdentity()) {
				baked = voxelutil::applyTransformToVolume(*volume, transform.worldMatrix(), node->pivot());
				if (!baked) {
					Log::error("Failed to bake vcdx layer transform for %s", layerName.c_str());
					return false;
				}
				volume = baked;
			}
		}
		wrapSave(priv::writeLayerVolume(brotli, volume))
		++layerIdx;
	}
	wrapSave(priv::writeEmptyRig(brotli, (int)models.size()))

	if (!brotli.flush()) {
		Log::error("Failed to finish vcdx brotli stream");
		return false;
	}
	return true;
}

#undef wrap
#undef wrapBool
#undef wrapImg
#undef wrapSave

} // namespace voxelformat
