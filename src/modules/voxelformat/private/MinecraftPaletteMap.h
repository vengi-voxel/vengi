/**
 * @file
 */

#include "core/collection/StringMap.h"

namespace voxelformat {

using PaletteMap = core::StringMap<int>;

// this list was found in enkiMI by Doug Binks and extended
const PaletteMap &getPaletteMap();

} // namespace voxelformat
