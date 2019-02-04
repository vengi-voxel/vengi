/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "math/Axis.h"
#include "../ModifierType.h"

namespace voxedit {
namespace tool {

extern bool aabb(voxel::RawVolume& target, const glm::ivec3& mins, const glm::ivec3& maxs, const voxel::Voxel& voxel, ModifierType modifierType, voxel::Region* modifiedRegion = nullptr);

}
}
