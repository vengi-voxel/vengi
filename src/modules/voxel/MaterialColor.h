/**
 * @file
 */

#pragma once

namespace palette {
class Palette;
}

namespace voxel {

void initPalette(const palette::Palette &palette);
palette::Palette& getPalette();
bool hasPalette();

}
