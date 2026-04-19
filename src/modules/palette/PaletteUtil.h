/**
 * @file
 */

#pragma once

#include "palette/ColorPalette.h"
#include "palette/Palette.h"

namespace palette {

palette::ColorPalette toColorPalette(const palette::Palette &palette);
palette::Palette toPalette(const palette::ColorPalette &colorPalette);

} // namespace palette
