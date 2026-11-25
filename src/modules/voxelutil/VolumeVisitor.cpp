/**
 * @file
 */

#include "VolumeVisitor.h"
#include "color/Distance.h"
#include "palette/Palette.h"

namespace voxelutil {

VisitVoxelFuzzyColor::VisitVoxelFuzzyColor(const palette::Palette &palette, uint8_t colorIndex, float threshold)
	: _palette(palette), _colorIndex(colorIndex), _threshold(threshold) {
}

float VisitVoxelFuzzyColor::getDistance(uint8_t colorIndex) const {
	const color::RGBA c1 = _palette.color(_colorIndex);
	const color::RGBA c2 = _palette.color(colorIndex);
	return color::getDistance(c1, c2, color::Distance::Approximation);
}

} // namespace voxelutil
