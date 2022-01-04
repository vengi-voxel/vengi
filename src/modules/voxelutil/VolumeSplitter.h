/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"

namespace voxel {

void splitVolume(const RawVolume *volume, const glm::ivec3 &maxSize, core::DynamicArray<voxel::RawVolume *> &rawVolumes);

} // namespace voxel
