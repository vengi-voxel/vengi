/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"

namespace voxedit {

/**
 * @brief Enumeration of available brush types in the voxel editor
 *
 * Each brush type provides a different way to place, modify, or select voxels in the scene.
 */
enum class BrushType { None, Shape, Plane, Stamp, Line, Path, Paint, Text, Select, Texture, Normal, Max };

/**
 * @brief String representation of brush types for UI display and command registration
 */
static constexpr const char *BrushTypeStr[] = {"None", "Shape", "Plane", "Stamp",  "Line",
											   "Path", "Paint", "Text",	 "Select", "Texture", "Normal"};
static_assert(lengthof(BrushTypeStr) == (int)BrushType::Max, "BrushTypeStr size mismatch");

} // namespace voxedit
