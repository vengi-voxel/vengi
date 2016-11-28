/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Voxel.h"
#include "core/Color.h"
#include "io/File.h"
#include <vector>
#include "core/Random.h"

namespace voxel {

// this size must match the color uniform size in the shader
typedef std::vector<glm::vec4> MaterialColorArray;

extern bool initDefaultMaterialColors();
extern bool initMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile);
extern const MaterialColorArray& getMaterialColors();
extern Voxel createRandomColorVoxel(VoxelType type);
extern Voxel createRandomColorVoxel(VoxelType type, core::Random& random);
/**
 * @brief Creates a voxel of the given type with the fixed colorindex that is relative to the
 * valid color indices for this type.
 */
extern Voxel createColorVoxel(VoxelType type, uint32_t colorIndex);

}
