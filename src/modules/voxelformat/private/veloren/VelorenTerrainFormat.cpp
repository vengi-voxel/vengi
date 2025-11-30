#include "VelorenTerrainFormat.h"

#include <SDL_stdinc.h>
#include "color/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "palette/RGBABuffer.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include <cstdint>

namespace voxelformat {

// Simple helper macros (same style as the cub example)
#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load veloren terrain file: Not enough data in stream " CORE_STRINGIFY(read));            \
		return 0;                                                                                                      \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error(                                                                                                    \
			"Could not load veloren terrain file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",       \
			(int)__LINE__);                                                                                            \
		return false;                                                                                                  \
	}

static inline uint64_t versionMagic(uint16_t version) {
	return ((uint64_t)version) | (0x3352ACEEA789UL << 16);
}

size_t VelorenTerrainFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
										 palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return 0;
	}

	uint64_t magicVersion = 0;
	wrap(stream->readUInt64(magicVersion))
	const int version = 3;
	if (versionMagic(version) != magicVersion) {
		Log::error("Not a veloren terrain file or unsupported version for file: %s", filename.c_str());
		return 0;
	}

	uint64_t count = 0;
	wrap(stream->readUInt64(count))
	palette::RGBABuffer colors;
	for (uint64_t i = 0; i < count; ++i) {
		if (version == 1) {
			int32_t x = 0, y = 0, z = 0;
			wrap(stream->readInt32(x));
			wrap(stream->readInt32(y));
			wrap(stream->readInt32(z));
		} else {
			uint8_t sx = 0, sy = 0;
			int16_t sz = 0;
			wrap(stream->readUInt8(sx));
			wrap(stream->readUInt8(sy));
			wrap(stream->readInt16(sz));
		}

		uint8_t blockType = BlockType::Air;
		color::RGBA rgba(0, 0, 0, 255);
		wrap(stream->readUInt8(blockType))
		wrap(stream->readUInt8(rgba.r))
		wrap(stream->readUInt8(rgba.g))
		wrap(stream->readUInt8(rgba.b))
		if (blockType == BlockType::Air) {
			continue;
		}
		colors.insert(rgba);
	}
	return createPalette(colors, palette);
}

bool VelorenTerrainFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
										  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
										  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	uint64_t magicVersion = 0;
	wrap(stream->readUInt64(magicVersion))
	const int version = 3;
	if (versionMagic(version) != magicVersion) {
		Log::error("Not a veloren terrain file or unsupported version for file: %s", filename.c_str());
		return 0;
	}

	uint64_t count = 0u;
	wrap(stream->readUInt64(count))

	palette::PaletteLookup palLookup(palette);
	voxel::SparseVolume v;
	for (uint64_t i = 0; i < count; ++i) {
		int32_t x = 0, y = 0, z = 0;
		if (version == 1) {
			wrap(stream->readInt32(x));
			wrap(stream->readInt32(y));
			wrap(stream->readInt32(z));
		} else {
			uint8_t sx = 0, sy = 0;
			int16_t sz = 0;
			wrap(stream->readUInt8(sx));
			wrap(stream->readUInt8(sy));
			wrap(stream->readInt16(sz));
			x = (int32_t)sx;
			y = (int32_t)sy;
			z = (int32_t)sz;
		}

		uint8_t blockType = BlockType::Air;
		color::RGBA rgba(0, 0, 0, 255);
		wrap(stream->readUInt8(blockType))
		wrap(stream->readUInt8(rgba.r))
		wrap(stream->readUInt8(rgba.g))
		wrap(stream->readUInt8(rgba.b))
		if (blockType == BlockType::Air) {
			// TODO: VOXELFORMAT: place the sprites, too - the latter 24 bits are the sprite types for air and water
			continue;
		}
		const int index = palLookup.findClosestIndex(rgba);
		const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
		v.setVoxel(x, z, -y, voxel); // swap y and z
	}

	int x = 0;
	int z = 0;
	core::String file = core::string::extractFilename(filename);
	if (SDL_sscanf(file.c_str(), "chunk_%i_%i.dat", &x, &z) == 2) {
		Log::debug("chunk position for %s at %i:%i", file.c_str(), x, z);
	}
	voxel::RawVolume *volume = new voxel::RawVolume(v.calculateRegion());
	v.copyTo(*volume);
	volume->region().shift(glm::ivec3(x * 32, 0, z * -32));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool VelorenTerrainFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
									  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	if (!node) {
		Log::error("No model node found in scenegraph");
		return false;
	}
	const voxel::RawVolume *volume = node->volume();
	if (!volume) {
		Log::error("No volume found in model node");
		return false;
	}
	const voxel::Region &region = volume->region();
	if (region.getWidthInVoxels() > 0xFF || region.getDepthInVoxels() > 0xFF) {
		Log::error("Volume is too large to be saved in veloren terrain format v3");
		return false;
	}
	if (region.getHeightInVoxels() > 0xFFFF) {
		Log::error("Volume is too tall to be saved in veloren terrain format v3");
		return false;
	}
	const palette::Palette &palette = node->palette();
	const uint32_t count = region.voxels();
	wrapBool(stream->writeUInt64(versionMagic(3)))
	wrapBool(stream->writeUInt64(count))

	for (uint32_t i = 0; i < count; ++i) {
		const glm::ivec3 pos = region.fromIndex(i);
		const voxel::Voxel voxel = volume->voxel(pos);
		const color::RGBA color = palette.color(voxel.getColor());
		uint8_t blockType = BlockType::Earth;
		if (voxel::isAir(voxel.getMaterial())) {
			blockType = BlockType::Air;
		}
		// write v3
		wrapBool(stream->writeUInt8(pos.x))
		wrapBool(stream->writeUInt8(region.getDepthInVoxels() - pos.z))
		wrapBool(stream->writeInt16(pos.y))

		wrapBool(stream->writeUInt8(blockType))
		wrapBool(stream->writeUInt8(color.r))
		wrapBool(stream->writeUInt8(color.g))
		wrapBool(stream->writeUInt8(color.b))
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
