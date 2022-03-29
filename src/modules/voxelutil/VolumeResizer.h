/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxelutil {

extern voxel::RawVolume *resize(const voxel::RawVolume *source, const glm::ivec3 &size, bool extendMins = false);

}
