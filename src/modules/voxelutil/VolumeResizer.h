/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxel {

extern voxel::RawVolume* resize(const voxel::RawVolume* source, const glm::ivec3& size);

}
