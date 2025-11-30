/**
 * @file
 */

#pragma once

#include "color/RGBA.h"

namespace core {

// Cyan, Magenta, Yellow und Key (Black)
struct CMYK {
	CMYK(float c, float m, float y, float k) : cmyk{c, m, y, k} {
	}
	float cmyk[4];

	CMYK &operator=(const CMYK &other);
	core::RGBA toRGB() const;
	static CMYK fromRGB(const core::RGBA &rgb);
};

} // namespace core
