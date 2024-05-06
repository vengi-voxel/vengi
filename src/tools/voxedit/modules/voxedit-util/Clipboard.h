/**
 * @file
 */

#pragma once

#include "modifier/Selection.h"
#include "voxel/VoxelData.h"

namespace voxedit {
namespace tool {

voxel::VoxelData copy(const voxel::VoxelData &voxelData, const Selections &selections);
voxel::VoxelData cut(voxel::VoxelData &voxelData, const Selections &selections, voxel::Region &modifiedRegion);
void paste(voxel::VoxelData &out, const voxel::VoxelData &in, const glm::ivec3 &referencePosition,
		   voxel::Region &modifiedRegion);

} // namespace tool
} // namespace voxedit
