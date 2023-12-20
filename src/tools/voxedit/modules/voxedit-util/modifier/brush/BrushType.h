/**
 * @file
 */

#include "core/ArrayLength.h"

namespace voxedit {

enum class BrushType { Shape, Plane, Stamp, Max };

static constexpr const char *BrushTypeStr[] = {"Shape", "Plane", "Stamp"};
static_assert(lengthof(BrushTypeStr) == (int)BrushType::Max, "BrushTypeStr size mismatch");

} // namespace voxedit
