/**
 * @file
 */

#pragma once

namespace math {

struct BVec4 {
	bool x; bool y; bool z; bool w;

	BVec4() = default;
	constexpr BVec4(bool x_, bool y_, bool z_, bool w_) : x(x_), y(y_), z(z_), w(w_) {}

	constexpr bool operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
};

inline constexpr bool all(const BVec4 &v) { return v.x && v.y && v.z && v.w; }
inline constexpr bool any(const BVec4 &v) { return v.x || v.y || v.z || v.w; }

}
