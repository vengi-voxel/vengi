/**
 * @file
 */

#pragma once

namespace palette {
class Palette;
}

namespace voxelui {

/**
 * Palette index field with a color swatch and a drop target for palette-panel drags.
 *
 * @param minIndex Inclusive lower bound (use -1 for "no color" / current voxel).
 * @param maxIndex Inclusive upper bound, or -1 to use the last palette slot when @p palette is set.
 * @param readOnly If @c true, the index cannot be edited or changed by drag-and-drop.
 * @return @c true if @p index changed
 */
bool paletteColorIndexInput(const char *label, int *index, const palette::Palette *palette = nullptr, int minIndex = 0,
							int maxIndex = -1, bool readOnly = false);

} // namespace voxelui
