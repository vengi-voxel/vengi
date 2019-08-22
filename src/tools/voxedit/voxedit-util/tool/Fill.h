/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolumeWrapper.h"
#include "math/Axis.h"
#include "../ModifierType.h"
#include "../Selection.h"

namespace voxedit {
namespace tool {

extern bool aabb(voxel::RawVolumeWrapper& target, const glm::ivec3& mins, const glm::ivec3& maxs, const voxel::Voxel& voxel, ModifierType modifierType, const Selection& selection, voxel::Region* modifiedRegion = nullptr);

extern voxel::RawVolume* copy(const voxel::RawVolume* volume, const Selection& selection);
extern voxel::RawVolume* cut(voxel::RawVolume* volume, const Selection& selection, voxel::Region& modifiedRegion);
extern void paste(voxel::RawVolume* out, const voxel::RawVolume* in, const glm::ivec3& referencePosition, voxel::Region& modifiedRegion);

}
}
