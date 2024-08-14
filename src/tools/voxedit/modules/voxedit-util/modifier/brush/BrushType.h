/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"

namespace voxedit {

enum class BrushType { None, Shape, Plane, Stamp, Line, Path, Paint, Text, Select, Max };

static constexpr const char *BrushTypeStr[] = {"None", "Shape", "Plane", "Stamp", "Line",
											   "Path", "Paint", "Text",	 "Select"};
static_assert(lengthof(BrushTypeStr) == (int)BrushType::Max, "BrushTypeStr size mismatch");

} // namespace voxedit
