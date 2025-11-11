/**
 * @file
 */

#pragma once

#include "modifier/SelectionManager.h"
#include "voxel/ClipboardData.h"

namespace voxedit {
namespace tool {

voxel::ClipboardData copy(const voxel::ClipboardData &voxelData, const SelectionManagerPtr &selectionMgr);
voxel::ClipboardData cut(voxel::ClipboardData &voxelData, const SelectionManagerPtr &selectionMgr, voxel::Region &modifiedRegion);
void paste(voxel::ClipboardData &out, const voxel::ClipboardData &in, const glm::ivec3 &referencePosition,
		   voxel::Region &modifiedRegion);

} // namespace tool
} // namespace voxedit
