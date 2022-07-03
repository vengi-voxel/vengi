/**
 * @file
 */

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

using PaletteMap = core::StringMap<McColor>;
using PaletteArray = core::BufferView<McColorScheme>;

// this list was found in enkiMI by Doug Binks and extended
const PaletteMap &getPaletteMap();
const PaletteArray &getPaletteArray();
/**
 * @return @c -1 (resp. the given default value) if not found - otherwise in the range [0-255]
 * @param[in] name mincraft:somename[parameters]
 * @param[in] defaultValue the value that is returned if the given name could not get matched
 */
int findPaletteIndex(const core::String &name, int defaultValue = -1);

} // namespace voxelformat
