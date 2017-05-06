#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "core/Axis.h"

namespace voxedit {
namespace tool {

extern bool fill(voxel::RawVolume& target, const glm::ivec3& position, const core::Axis axis, const voxel::Voxel& voxel, bool overwrite = true, voxel::Region* modifiedRegion = nullptr);

}
}
