/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "math/Axis.h"

namespace voxedit {
namespace tool {

extern bool fill(voxel::RawVolume& target, const glm::ivec3& position, const math::Axis axis, const voxel::Voxel& voxel, bool overwrite = true, voxel::Region* modifiedRegion = nullptr);

extern bool aabb(voxel::RawVolume& target, const glm::ivec3& mins, const glm::ivec3& maxs, const voxel::Voxel& voxel, bool overwrite = true, voxel::Region* modifiedRegion = nullptr);

}
}
