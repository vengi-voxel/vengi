/**
 * @file
 */

#pragma once

#include "tb_types.h"
#include "core/GLM.h"

namespace tb {

/** TBColor contains a 32bit color. */

// TODO: replace with glm::u8vec4
class TBColor {
public:
	constexpr TBColor() : b(0), g(0), r(0), a(255) {
	}
	constexpr TBColor(int r, int g, int b, int a = 255) : b(b), g(g), r(r), a(a) {
	}

	static TBColor fromVec4(const glm::vec4& c);

	static TBColor fromVec3(const glm::vec3& c);

	uint8_t b, g, r, a;

	void set(const TBColor &color) {
		*this = color;
	}

	/** Set the color from string in any of the following formats:
		"#rrggbbaa", "#rrggbb", "#rgba", "#rgb" */
	void setFromString(const char *str, int len);

	inline operator uint32_t() const {
		return *((uint32_t *)this);
	}
	inline bool operator==(const TBColor &c) const {
		return *this == (uint32_t)c;
	}
	inline bool operator!=(const TBColor &c) const {
		return !(*this == c);
	}
};

} // namespace tb
