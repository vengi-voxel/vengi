/**
 * @file
 */

#include "Voxel.h"
#include "core/Assert.h"
#include <SDL_stdinc.h>

namespace voxel {

VoxelType getVoxelType(const char *str) {
	for (int j = 0; j < (int)voxel::VoxelType::Max; ++j) {
		if (SDL_strcmp(VoxelTypeStr[j], str) != 0) {
			continue;
		}
		return (VoxelType)j;
	}
	return VoxelType::Max;
}

void Voxel::setFlags(uint8_t flags) {
	_flags = flags;
	// max 3 bits
	core_assert(flags <= 7);
}

}
