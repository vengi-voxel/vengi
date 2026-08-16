/**
 * @file
 */

#include "Util.h"
#include "../MinecraftPaletteMap.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "palette/Palette.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxelformat {
namespace schematic {

glm::ivec3 parsePosList(const priv::NamedBinaryTag &compound, const core::String &key) {
	const priv::NamedBinaryTag &pos = compound.get(key);
	int x = -1;
	int y = -1;
	int z = -1;
	if (pos.type() == priv::TagType::LIST) {
		const priv::NBTList &positions = *pos.list();
		if (positions.size() != 3) {
			Log::error("Unexpected nbt %s list entry count: %i", key.c_str(), (int)positions.size());
			return glm::ivec3(-1);
		}
		x = positions[0].int32(-1);
		y = positions[1].int32(-1);
		z = positions[2].int32(-1);
	} else if (pos.type() == priv::TagType::COMPOUND) {
		x = pos.get("x").int32(-1);
		y = pos.get("y").int32(-1);
		z = pos.get("z").int32(-1);
	}
	return glm::ivec3(x, y, z);
}

void setSchematicPaletteEntry(SchematicPalette &colors, SchematicWaterPalette &water, int idx,
							  const core::String &blockName) {
	if (idx >= (int)colors.size()) {
		colors.resize(idx + 1);
	}
	if (idx >= (int)water.size()) {
		water.resize(idx + 1);
	}
	colors[idx] = findPaletteIndex(blockName, 1);
	water[idx] = isWaterBlock(blockName) ? 1u : 0u;
}

bool schematicPaletteEntryIsWater(const SchematicWaterPalette &water, int paletteIdx) {
	if (paletteIdx < 0 || paletteIdx >= (int)water.size()) {
		return false;
	}
	return water[paletteIdx] != 0u;
}

voxel::Voxel createSchematicVoxel(const palette::Palette &palette, uint8_t colorIdx, bool water) {
	if (core::getVar(cfg::VoxformatMCSeparateWater)->boolVal() && water) {
		return voxel::createVoxel(voxel::VoxelType::Transparent, colorIdx);
	}
	return voxel::createVoxel(palette, colorIdx);
}

bool schematicLegacyBlockIsWater(int blockId) {
	return isLegacyWaterId(blockId);
}

} // namespace schematic
} // namespace voxelformat
