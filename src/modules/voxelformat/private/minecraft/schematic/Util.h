/**
 * @file
 */

#pragma once

#include "../NamedBinaryTag.h"
#include "core/collection/Buffer.h"
#include <glm/fwd.hpp>

namespace palette {
class Palette;
}

namespace voxel {
class Voxel;
}

namespace voxelformat {
namespace schematic {

using SchematicPalette = core::Buffer<int>;
using SchematicWaterPalette = core::Buffer<uint8_t>;

glm::ivec3 parsePosList(const priv::NamedBinaryTag &compound, const core::String &key);
void setSchematicPaletteEntry(SchematicPalette &colors, SchematicWaterPalette &water, int idx, const core::String &blockName);
bool schematicPaletteEntryIsWater(const SchematicWaterPalette &water, int paletteIdx);
voxel::Voxel createSchematicVoxel(const palette::Palette &palette, uint8_t colorIdx, bool water);
bool schematicLegacyBlockIsWater(int blockId);

} // namespace schematic
} // namespace voxelformat
