/**
 * @file
 */

#pragma once

namespace math {

struct BVec2 {
	bool x; bool y;

	BVec2() = default;
	constexpr BVec2(bool x_, bool y_) : x(x_), y(y_) {}

	constexpr bool operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
};

inline constexpr bool all(const BVec2 &v) { return v.x && v.y; }
inline constexpr bool any(const BVec2 &v) { return v.x || v.y; }

}
