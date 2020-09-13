/**
 * @file
 */

#include "KV6Format.h"
#include "voxel/MaterialColor.h"
#include "io/FileStream.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include <glm/common.hpp>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

bool KV6Format::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load kv6 file: File doesn't exist");
		return false;
	}
	io::FileStream stream(file.get());

	uint32_t magic;
	wrap(stream.readInt(magic))
	if (magic != FourCC('K','v','x','l')) {
		Log::error("Invalid magic");
		return false;
	}

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz, ysiz, zsiz;
	wrap(stream.readInt(xsiz))
	wrap(stream.readInt(ysiz))
	wrap(stream.readInt(zsiz))

	if (xsiz > 256 || ysiz > 256 || zsiz > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", xsiz, zsiz, ysiz);
		return false;
	}

	/**
	 * Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	 */
	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))

	if (xsiz > MaxRegionSize || ysiz > MaxRegionSize || zsiz > MaxRegionSize) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}
	const voxel::Region region(0, 0, 0, xsiz - 1, zsiz - 1, ysiz - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}

	uint32_t numvoxs;
	wrap(stream.readInt(numvoxs))
	Log::debug("numvoxs: %u", numvoxs);
	constexpr uint32_t MAXVOXS = 1048576;
	if (numvoxs > MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, MAXVOXS);
		return false;
	}

	core_assert(stream.pos() == 32);
	if (stream.seek(32 + numvoxs * 8 + (xsiz << 2) + ((xsiz * ysiz) << 1)) != -1) {
		if (stream.remaining() != 0) {
			uint32_t palMagic;
			wrap(stream.readInt(palMagic))
			if (palMagic == FourCC('S','P','a','l')) {
				_paletteSize = _palette.size();
				const MaterialColorArray& materialColors = getMaterialColors();
				for (size_t i = 0; i < _paletteSize; ++i) {
					uint8_t r, g, b;
					wrap(stream.readByte(b))
					wrap(stream.readByte(g))
					wrap(stream.readByte(r))

					const uint8_t nr = glm::clamp((uint32_t)glm::round((r * 255) / 63.0f), 0u, 255u);
					const uint8_t ng = glm::clamp((uint32_t)glm::round((g * 255) / 63.0f), 0u, 255u);
					const uint8_t nb = glm::clamp((uint32_t)glm::round((b * 255) / 63.0f), 0u, 255u);

					const glm::vec4& color = core::Color::fromRGBA(nr, ng, nb, 255u);
					const int index = core::Color::getClosestMatch(color, materialColors);
					_palette[i] = index;
				}
			}
		}
	}
	stream.seek(32);

	RawVolume *volume = new RawVolume(region);
	volumes.push_back(VoxelVolume{volume, file->fileName(), true});

	typedef struct {
		uint8_t z, col, vis, dir;
	} voxtype;

	voxtype voxdata[MAXVOXS];
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		uint8_t palr, palg, palb, pala;
		wrap(stream.readByte(palb))
		wrap(stream.readByte(palg))
		wrap(stream.readByte(palr))
		wrap(stream.readByte(pala))
		const glm::vec4& color = core::Color::fromRGBA(palr, palg, palb, pala);
		voxdata[c].col = findClosestIndex(color);
		uint16_t zpos;
		wrap(stream.readShort(zpos))
		voxdata[c].z = zpos;
		wrap(stream.readByte(voxdata[c].vis))
		wrap(stream.readByte(voxdata[c].dir))
	}
	stream.skip(xsiz * sizeof(uint32_t));

	uint16_t xyoffset[256][256];
	for (uint32_t x = 0u; x < xsiz; ++x) {
		for (uint32_t y = 0u; y < ysiz; ++y) {
			wrap(stream.readShort(xyoffset[x][y]))
		}
	}

	int idx = 0;
	for (uint32_t x = 0; x < xsiz; ++x) {
		for (uint32_t y = 0; y < ysiz; ++y) {
			for (int end = idx + xyoffset[x][y]; idx < end; ++idx) {
				const voxtype& vox = voxdata[idx];
				const voxel::Voxel col = voxel::createVoxel(voxel::VoxelType::Generic, vox.col);
				volume->setVoxel(x, (zsiz - 1) - vox.z, y, col);
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
						volume->setVoxel(x, (zsiz - 1) - lastZ, y, lastCol);
					}
				}
			}
		}
	}
	return true;
}

#undef wrap

bool KV6Format::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	return false;
}

}
