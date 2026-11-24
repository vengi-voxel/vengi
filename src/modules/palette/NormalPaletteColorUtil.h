/**
 * @file
 */

#pragma once

#include "math/Vec4f.h"
#include "palette/NormalPalette.h"

namespace palette {

/**
 * @note The target buffer must at least have space for 256 entries
 */
void toVec4f(const NormalPalette &palette, math::Vec4f *vec4f);

} // namespace palette
