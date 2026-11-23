/**
 * @file
 */

#pragma once

namespace math {

struct BVec3 {
	bool x; bool y; bool z;

	BVec3() = default;
	constexpr BVec3(bool x_, bool y_, bool z_) : x(x_), y(y_), z(z_) {}

	constexpr bool operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
};

inline constexpr bool all(const BVec3 &v) { return v.x && v.y && v.z; }
inline constexpr bool any(const BVec3 &v) { return v.x || v.y || v.z; }

}
