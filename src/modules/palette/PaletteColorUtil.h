/**
 * @file
 */

#pragma once

#include "math/Vec4f.h"
#include "palette/Palette.h"

namespace palette {

math::Vec4f color4(const Palette &palette, uint8_t paletteColorIdx);

/**
 * @brief Combined conversion of palette colors and emit colors in a single pass for better cache locality
 * @note Both target buffers must at least have space for 256 entries
 */
void toVec4f(const Palette &palette, math::Vec4f *materialColors, math::Vec4f *emitColors);

} // namespace palette
