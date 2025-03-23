/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace palette {
namespace adobe {

enum ColorSpace : uint16_t {
	RGB = 0,
	HSB = 1,
	CMYK = 2,
	Pantone = 3,
	Focoltone = 4,
	Trumatch = 5,
	Toyo = 6,
	Lab = 7, // CIELAB D50
	Grayscale = 8,
	HKS = 10,
	Max
};

} // namespace adobe
} // namespace palette
