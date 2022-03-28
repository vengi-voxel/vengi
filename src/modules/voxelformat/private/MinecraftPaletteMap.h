/**
 * @file
 */

#include "core/collection/StringMap.h"
#include "core/collection/BufferView.h"

namespace voxelformat {

struct McColorScheme {
	const char *name;
	int palIdx;
};

using PaletteMap = core::StringMap<int>;
using PaletteArray = core::BufferView<McColorScheme>;

// this list was found in enkiMI by Doug Binks and extended
const PaletteMap &getPaletteMap();
const PaletteArray &getPaletteArray();

} // namespace voxelformat
