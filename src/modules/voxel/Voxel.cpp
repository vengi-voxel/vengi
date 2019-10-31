/**
 * @file
 */

#include "Voxel.h"
#include <string.h>

namespace voxel {

VoxelType getVoxelType(const char *str) {
	for (int j = 0; j < (int)voxel::VoxelType::Max; ++j) {
		if (strcmp(VoxelTypeStr[j], str) != 0) {
			continue;
		}
		return (VoxelType)j;
	}
	return VoxelType::Max;
}

}
