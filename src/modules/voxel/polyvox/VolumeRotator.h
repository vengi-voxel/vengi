/**
 * @file
 */

#pragma once

#include "RawVolume.h"

namespace voxel {

extern RawVolume* rotateVolume(const RawVolume* source, const glm::vec3& angles, bool increaseSize = true);

}
