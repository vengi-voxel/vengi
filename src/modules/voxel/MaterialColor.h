/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include "core/io/File.h"
#include "image/Image.h"

#include <glm/vec4.hpp>
#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace math {
class Random;
}

namespace voxel {

// this size must match the color uniform size in the shader
typedef core::DynamicArray<glm::vec4> MaterialColorArray;
typedef core::DynamicArray<uint8_t> MaterialColorIndices;

extern const char* getDefaultPaletteName();
extern core::String extractPaletteName(const core::String& file);

extern bool initDefaultMaterialColors();
extern bool initMinecraftMaterialColors();
extern bool initMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile);
extern bool initMaterialColors(const uint8_t* paletteBuffer, size_t paletteBufferSize, const core::String& luaBuffer);
extern bool overrideMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile);
extern bool overrideMaterialColors(const uint8_t* paletteBuffer, size_t paletteBufferSize, const core::String& luaBuffer);
extern void shutdownMaterialColors();
extern void materialColorMarkClean();
extern bool materialColorChanged();
extern const MaterialColorArray& getMaterialColors();
extern const glm::vec4& getMaterialColor(const Voxel& voxel);

extern bool createPalette(const image::ImagePtr& image, uint32_t *colorsBuffer, int colors);
extern bool createPaletteFile(const image::ImagePtr& image, const char *paletteFile);

/**
 * @brief Get all known material color indices for the given VoxelType
 * @param type The VoxelType to get the indices for
 * @return Indices to the MaterialColorArray for the given VoxelType
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
