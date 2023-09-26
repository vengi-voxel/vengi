/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/fwd.hpp>

namespace voxel {
class RawVolume;
} // namespace voxel

namespace voxelutil {

void splitVolume(const voxel::RawVolume *volume, const glm::ivec3 &maxSize,
				 core::DynamicArray<voxel::RawVolume *> &rawVolumes, bool createEmpty = false);

void splitObjects(const voxel::RawVolume *v, core::DynamicArray<voxel::RawVolume *> &rawVolumes, VisitorOrder order = VisitorOrder::ZYX);

} // namespace voxelutil
