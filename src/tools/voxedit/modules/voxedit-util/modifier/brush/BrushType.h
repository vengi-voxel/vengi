/**
 * @file
 */

#include "core/ArrayLength.h"

namespace voxedit {

enum class BrushType { None, Shape, Plane, Stamp, Max };

static constexpr const char *BrushTypeStr[] = {"None", "Shape", "Plane", "Stamp"};
static_assert(lengthof(BrushTypeStr) == (int)BrushType::Max, "BrushTypeStr size mismatch");

} // namespace voxedit
