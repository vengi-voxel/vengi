/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Voxel.h"
#include "core/Color.h"
#include "io/File.h"
#include <vector>

namespace voxel {

// this size must match the color uniform size in the shader
typedef std::vector<glm::vec4> MaterialColorArray;

extern bool initDefaultMaterialColors();
extern bool initMaterialColors(const io::FilePtr& file);
extern const MaterialColorArray& getMaterialColors();
extern Voxel createRandomColorVoxel(VoxelType type);

}
