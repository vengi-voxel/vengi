/**
 * @file
 */

#pragma once

#include "palette/ColorPalette.h"
#include "palette/Palette.h"

namespace palette {

palette::ColorPalette toColorPalette(const palette::Palette &palette);
palette::Palette toPalette(const palette::ColorPalette &colorPalette);
/**
 * @brief Like toPalette(), but quantizes to at most @c targetColors colors when the palette exceeds
 * PaletteMaxColors. If @c targetColors is <= 0, the full PaletteMaxColors limit is used.
 */
palette::Palette toPaletteQuantized(const palette::ColorPalette &colorPalette, int targetColors);

} // namespace palette
