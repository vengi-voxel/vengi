/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/StringMap.h"
#include "core/collection/BufferView.h"

namespace voxelformat {

struct McColorScheme {
	const char *name;
	int palIdx;
	uint8_t alpha;
};

struct McColor {
	int palIdx;
	uint8_t alpha;
};

using McPaletteMap = core::StringMap<McColor>;
using McPaletteArray = core::BufferView<McColorScheme>;

// this list was found in enkiMI by Doug Binks and extended
// https://github.com/PrismarineJS/minecraft-data
// https://github.com/spoutn1k/mcmap
const McPaletteMap &getPaletteMap();
const McPaletteArray &getPaletteArray();

struct McBlock {
	core::String original; // e.g. "minecraft:stone[lit=true][INT] = 554"
	core::String blockId;  // e.g. "minecraft:stone"
	core::String biomeId;  // e.g. "minecraft:badlands"
	int8_t lit = -1;
	core::StringMap<core::String> properties{32};

	core::String normalize() const {
		if (biomeId.empty() && lit == -1) {
			return blockId;
		}
		core::String normalized = blockId;
		if (!biomeId.empty()) {
			normalized.append(",biome=").append(biomeId);
		}
		if (lit != -1) {
			normalized.append(",lit=").append(lit == 1 ? "true" : "false");
		}
		return normalized;
	}

	// hash
	size_t operator() (const McBlock& o) const {
		return core::StringHash()(blockId) ^ core::StringHash()(biomeId);
	}

	// compare
	bool operator==(const McBlock& o) const {
		return blockId == o.blockId && biomeId == o.biomeId;
	}
};
McBlock parseBlock(const core::String &blockId);
/**
 * @return @c -1 (resp. the given default value) if not found - otherwise in the range [0-255]
 * @param[in] name mincraft:somename[parameters]
 * @param[in] defaultValue the value that is returned if the given name could not get matched
 */
int findPaletteIndex(const core::String &name, int defaultValue = -1);
core::String findPaletteName(int palIdx);

} // namespace voxelformat
