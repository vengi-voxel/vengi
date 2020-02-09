/**
 * @file
 */

#include "Voxel.h"
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

}
