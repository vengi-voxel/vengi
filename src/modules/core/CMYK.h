/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include <glm/vec4.hpp>

namespace core {

// Cyan, Magenta, Yellow und Key (Black)
struct CMYK {
	CMYK(float c, float m, float y, float k) : cmyk{c, m, y, k} {
	}
	glm::vec4 cmyk;

	inline CMYK &operator=(const CMYK &other) {
		cmyk = other.cmyk;
		return *this;
	}
	core::RGBA toRGB() const;
	static CMYK fromRGB(const core::RGBA &rgb);
};

} // namespace core
