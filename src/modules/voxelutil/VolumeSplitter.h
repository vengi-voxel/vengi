/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include <glm/fwd.hpp>

namespace voxel {
class RawVolume;
} // namespace voxel

namespace voxelutil {

void splitVolume(const voxel::RawVolume *volume, const glm::ivec3 &maxSize,
				 core::DynamicArray<voxel::RawVolume *> &rawVolumes, bool createEmpty = false);

} // namespace voxelutil
