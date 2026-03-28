/**
 * @file
 */

#include "Voxel.h"
#include "core/Assert.h"
#include "palette/Palette.h"

namespace voxel {

#ifdef VENGI_COMPACT_VOXEL

void Voxel::setFlags(uint8_t flags) {
	if (flags & FlagOutline) {
		_data |= FLAG_OUTLINE_MASK;
	} else {
		_data &= ~FLAG_OUTLINE_MASK;
	}
}

#else // !VENGI_COMPACT_VOXEL

void Voxel::setFlags(uint8_t flags) {
	_flags = flags;
	// max 3 bits
	core_assert(flags <= 7);
}

#endif // VENGI_COMPACT_VOXEL

voxel::Voxel createVoxelFromColor(const palette::Palette &pal, color::RGBA color) {
	if (color.rgba == 0) {
		return {};
	}
	int idx = pal.getClosestMatch(color);
	if (idx == palette::PaletteColorNotFound) {
		return {};
	}
	return createVoxel(pal, idx);
}

voxel::Voxel createVoxel(const palette::Palette &pal, uint8_t index, uint8_t normalIndex, uint8_t flags, uint8_t boneIdx) {
	if (index < pal.size()) {
		const color::RGBA color = pal.color(index);
		if (color.rgba == 0) {
			return {};
		}
#ifndef VENGI_COMPACT_VOXEL
		if (color.a != 255) {
			return createVoxel(VoxelType::Transparent, index, normalIndex, flags, boneIdx);
		}
#endif
	}
	return createVoxel(VoxelType::Generic, index, normalIndex, flags, boneIdx);
}

}
