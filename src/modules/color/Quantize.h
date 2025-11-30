/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include <stddef.h>

namespace color {

enum class ColorReductionType {
	Octree,
	Wu,
	MedianCut,
	KMeans,
	NeuQuant,

	Max
};

ColorReductionType toColorReductionType(const char *str);
const char *toColorReductionTypeString(ColorReductionType type);

/**
 * @return @c -1 on error or the amount of @code colors <= maxTargetBufColors @endcode
 */
int quantize(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors,
			 ColorReductionType type = ColorReductionType::MedianCut);

} // namespace color
