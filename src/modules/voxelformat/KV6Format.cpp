/**
 * @file
 */

#include "KV6Format.h"
#include "io/Stream.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "voxel/PaletteLookup.h"
#include "voxel/Palette.h"
#include <glm/common.hpp>

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool KV6Format::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('K','v','x','l')) {
		Log::error("Invalid magic");
		return false;
	}

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz, ysiz, zsiz;
	wrap(stream.readUInt32(xsiz))
	wrap(stream.readUInt32(ysiz))
	wrap(stream.readUInt32(zsiz))

	if (xsiz > 256 || ysiz > 256 || zsiz > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", xsiz, zsiz, ysiz);
		return false;
	}

	/**
	 * Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	 */
	SceneGraphTransform transform;
	glm::vec3 normalizedPivot;
	wrap(stream.readFloat(normalizedPivot.x))
	wrap(stream.readFloat(normalizedPivot.y))
	wrap(stream.readFloat(normalizedPivot.z))
	transform.setPivot(normalizedPivot);

	if (xsiz > MaxRegionSize || ysiz > MaxRegionSize || zsiz > MaxRegionSize) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}
	const voxel::Region region(0, 0, 0, (int)xsiz - 1, (int)zsiz - 1, (int)ysiz - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}

	uint32_t numvoxs;
	wrap(stream.readUInt32(numvoxs))
	Log::debug("numvoxs: %u", numvoxs);
	constexpr uint32_t MAXVOXS = 1048576;
	if (numvoxs > MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, MAXVOXS);
		return false;
	}

	if (stream.seek(32 + numvoxs * 8 + (xsiz << 2) + ((xsiz * ysiz) << 1)) != -1) {
		if (stream.remaining() != 0) {
			uint32_t palMagic;
			wrap(stream.readUInt32(palMagic))
			if (palMagic == FourCC('S','P','a','l')) {
				palette.colorCount = voxel::PaletteMaxColors;
				for (int i = 0; i < palette.colorCount; ++i) {
					uint8_t r, g, b;
					wrap(stream.readUInt8(b))
					wrap(stream.readUInt8(g))
					wrap(stream.readUInt8(r))

					const uint8_t nr = glm::clamp((uint32_t)glm::round(((float)r * 255.0f) / 63.0f), 0u, 255u);
					const uint8_t ng = glm::clamp((uint32_t)glm::round(((float)g * 255.0f) / 63.0f), 0u, 255u);
					const uint8_t nb = glm::clamp((uint32_t)glm::round(((float)b * 255.0f) / 63.0f), 0u, 255u);

					const glm::vec4& color = core::Color::fromRGBA(nr, ng, nb, 255u);
					palette.colors[i] = core::Color::getRGBA(color);
				}
			}
		}
	}
	stream.seek(32);

	typedef struct {
		uint8_t z, col, vis, dir;
	} voxtype;

	voxtype voxdata[MAXVOXS];
	voxel::PaletteLookup palLookup(palette);
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		uint8_t palr, palg, palb, pala;
		wrap(stream.readUInt8(palb))
		wrap(stream.readUInt8(palg))
		wrap(stream.readUInt8(palr))
		wrap(stream.readUInt8(pala))
		const glm::vec4& color = core::Color::fromRGBA(palr, palg, palb, pala);
		voxdata[c].col = palLookup.findClosestIndex(color);
		uint16_t zpos;
		wrap(stream.readUInt16(zpos))
		voxdata[c].z = zpos;
		wrap(stream.readUInt8(voxdata[c].vis))
		wrap(stream.readUInt8(voxdata[c].dir))
	}
	stream.skip((int64_t)(xsiz * sizeof(uint32_t)));

	uint16_t xyoffset[256][256];
	for (uint32_t x = 0u; x < xsiz; ++x) {
		for (uint32_t y = 0u; y < ysiz; ++y) {
			wrap(stream.readUInt16(xyoffset[x][y]))
		}
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);

	int idx = 0;
	for (uint32_t x = 0; x < xsiz; ++x) {
		for (uint32_t y = 0; y < ysiz; ++y) {
			for (int end = idx + xyoffset[x][y]; idx < end; ++idx) {
				const voxtype& vox = voxdata[idx];
				const voxel::Voxel col = voxel::createVoxel(voxel::VoxelType::Generic, vox.col);
				volume->setVoxel((int)x, (int)((zsiz - 1) - vox.z), (int)y, col);
			}
		}
	}

	uint32_t lastZ = 256;
	voxel::Voxel lastCol;
	idx = 0;
	for (uint32_t x = 0; x < xsiz; ++x) {
		for (uint32_t y = 0; y < ysiz; ++y) {
			for (int end = idx + xyoffset[x][y]; idx < end; ++idx) {
				const voxtype& vox = voxdata[idx];
				if (vox.vis & (1 << 4)) {
					lastZ = vox.z;
					lastCol = voxel::createVoxel(voxel::VoxelType::Generic, vox.col);
				}
				if (vox.vis & (1 << 5)) {
					for (; lastZ < vox.z; ++lastZ) {
						volume->setVoxel((int)x, (int)((zsiz - 1) - lastZ), (int)y, lastCol);
					}
				}
			}
		}
	}

	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	transform.update(sceneGraph, node, 0);
	node.setTransform(0, transform);
	node.setPalette(palLookup.palette());
	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap

bool KV6Format::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
}

}
