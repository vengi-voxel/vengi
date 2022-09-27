/**
 * @file
 */

#include "KV6Format.h"
#include "core/Common.h"
#include "io/Stream.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "voxel/PaletteLookup.h"
#include "voxel/Palette.h"
#include "voxelformat/SceneGraphNode.h"
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

	SceneGraphTransform transform;
	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))

	pivot.z = (float)zsiz - 1.0f - pivot.z;

	glm::vec3 normalizedPivot = pivot / glm::vec3(xsiz, ysiz, zsiz);
	core::exchange(normalizedPivot.y, normalizedPivot.z);
	transform.setPivot(normalizedPivot);

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

	const int64_t headerSize = 32;
	const int64_t xLenSize = (int64_t)(xsiz * sizeof(uint32_t));
	const int64_t yLenSize = (int64_t)((size_t)(xsiz * ysiz) * sizeof(uint16_t));
	const int64_t paletteOffset = headerSize + (int64_t)(numvoxs * 8) + xLenSize + yLenSize;
	if (stream.seek(paletteOffset) != -1) {
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
	stream.seek(headerSize);

	typedef struct {
		uint8_t z_low;	// z coordinate of this surface voxel (height)
		uint8_t z_high; // z coordinate of this surface voxel (height)
		uint8_t col;	// palette index
		uint8_t vis;	// Low 6 bits say if neighbor is solid or air
		uint8_t dir;	// Uses 256-entry lookup table
	} voxtype;

	voxtype voxdata[MAXVOXS];
	voxel::PaletteLookup palLookup(palette);
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		uint8_t palr, palg, palb, pala;
		wrap(stream.readUInt8(palb))
		wrap(stream.readUInt8(palg))
		wrap(stream.readUInt8(palr))
		wrap(stream.readUInt8(pala)) // always 128
		const glm::vec4& color = core::Color::fromRGBA(palr, palg, palb, 255);
		voxdata[c].col = palLookup.findClosestIndex(color);
		wrap(stream.readUInt8(voxdata[c].z_low))
		wrap(stream.readUInt8(voxdata[c].z_high))
		wrap(stream.readUInt8(voxdata[c].vis))
		wrap(stream.readUInt8(voxdata[c].dir))
	}
	stream.skip(xLenSize);

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
				volume->setVoxel((int)x, (int)((zsiz - 1) - vox.z_low), (int)y, col);
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
					lastZ = vox.z_low;
					lastCol = voxel::createVoxel(voxel::VoxelType::Generic, vox.col);
				}
				if (vox.vis & (1 << 5)) {
					for (; lastZ < vox.z_low; ++lastZ) {
						volume->setVoxel((int)x, (int)((zsiz - 1) - lastZ), (int)y, lastCol);
					}
				}
			}
		}
	}

	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palLookup.palette());
	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap

bool KV6Format::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
}

}
