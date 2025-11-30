/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include <stddef.h>

namespace color {

/**
 * @return @c -1 on error or the amount of @code colors <= maxTargetBufColors @endcode
 */
int quantize(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors,
			 Color::ColorReductionType type = Color::ColorReductionType::MedianCut);

} // namespace color
