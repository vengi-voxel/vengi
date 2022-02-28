/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include "voxel/Palette.h"

namespace math {
class Random;
}

namespace voxel {

extern bool initDefaultPalette();
extern bool initPalette(const voxel::Palette &palette);
extern bool overridePalette(const voxel::Palette &palette);
extern void shutdownMaterialColors();
extern void materialColorMarkClean();
extern bool materialColorChanged();
extern Palette& getPalette();

// this size must match the color uniform size in the shader
typedef core::DynamicArray<uint8_t> MaterialColorIndices;

/**
 * @brief Get all known material color indices for the given VoxelType
 * @param type The VoxelType to get the indices for
 * @return Indices to the palette color array for the given VoxelType
 */
extern const MaterialColorIndices& getMaterialIndices(VoxelType type);
extern Voxel createRandomColorVoxel(VoxelType type);
extern Voxel createRandomColorVoxel(VoxelType type, math::Random& random);
/**
 * @brief Creates a voxel of the given type with the fixed colorindex that is relative to the
 * valid color indices for this type.
 */
extern Voxel createColorVoxel(VoxelType type, uint32_t colorIndex);

}
