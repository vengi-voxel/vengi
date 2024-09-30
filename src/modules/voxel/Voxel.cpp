/**
 * @file
 */

#include "Voxel.h"
#include "core/Assert.h"
#include "palette/Palette.h"
#include <SDL_stdinc.h>

namespace voxel {

void Voxel::setFlags(uint8_t flags) {
	_flags = flags;
	// max 3 bits
	core_assert(flags <= 7);
}

voxel::Voxel createVoxel(const palette::Palette &pal, uint8_t index, uint8_t normalIndex, uint8_t flags) {
	if (index < pal.size()) {
		const core::RGBA color = pal.color(index);
		if (color.rgba == 0) {
			return {};
		}
		if (color.a != 255) {
			return createVoxel(VoxelType::Transparent, index, normalIndex, flags);
		}
	}
	return createVoxel(VoxelType::Generic, index, normalIndex, flags);
}

}
