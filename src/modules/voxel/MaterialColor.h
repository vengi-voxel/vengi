/**
 * @file
 */

#pragma once

#include "core/Color.h"
#include <vector>

namespace voxel {

// this size must match the color uniform size in the shader
typedef std::vector<glm::vec4> MaterialColorArray;

extern const MaterialColorArray& getMaterialColors();

}
