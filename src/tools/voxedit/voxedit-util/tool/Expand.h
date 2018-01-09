/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"

namespace voxedit {
namespace tool {

extern voxel::RawVolume* expand(const voxel::RawVolume* source, const glm::ivec3& size);

}
}
