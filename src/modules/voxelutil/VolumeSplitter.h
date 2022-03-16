/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"

namespace voxelutil {

void splitVolume(const voxel::RawVolume *volume, const glm::ivec3 &maxSize,
				 core::DynamicArray<voxel::RawVolume *> &rawVolumes);

} // namespace voxelutil
