/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include "core/collection/DynamicArray.h"

namespace voxel {

class Palette;

extern bool initDefaultPalette();
extern bool initPalette(const voxel::Palette &palette);
extern bool overridePalette(const voxel::Palette &palette);
extern void shutdownMaterialColors();
extern Palette& getPalette();

// this size must match the color uniform size in the shader
typedef core::DynamicArray<uint8_t> MaterialColorIndices;

/**
 * @brief Get all known material color indices for the given VoxelType
 * @param type The VoxelType to get the indices for
 * @return Indices to the palette color array for the given VoxelType
 */
extern const MaterialColorIndices& getMaterialIndices(VoxelType type);
/**
 * @brief Creates a voxel of the given type with the fixed colorindex that is relative to the
 * valid color indices for this type.
 */
extern Voxel createColorVoxel(VoxelType type, uint32_t colorIndex);

}
