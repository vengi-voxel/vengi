/**
 * @file
 */

#pragma once

#include "color/RGBA.h"

namespace color {

// Cyan, Magenta, Yellow und Key (Black)
struct CMYK {
	CMYK(float c, float m, float y, float k) : cmyk{c, m, y, k} {
	}
	float cmyk[4];

	CMYK &operator=(const CMYK &other);
	color::RGBA toRGB() const;
	static CMYK fromRGB(const color::RGBA &rgb);
};

} // namespace color
