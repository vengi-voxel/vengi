/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace math {

struct U8Vec3 {
	union {
		struct { uint8_t x, y, z; };
		struct { uint8_t r, g, b; };
	};
	typedef U8Vec3 col_type;
	typedef uint8_t value_type;
	static constexpr int length() {
		return 3;
	}

	U8Vec3() = default;
	constexpr U8Vec3(uint8_t s) : x(s), y(s), z(s) {
	}
	constexpr U8Vec3(uint8_t x_, uint8_t y_, uint8_t z_) : x(x_), y(y_), z(z_) {
	}

	constexpr U8Vec3 operator+(const U8Vec3 &v) const {
		return U8Vec3((uint8_t)(x + v.x), (uint8_t)(y + v.y), (uint8_t)(z + v.z));
	}
	constexpr U8Vec3 operator-(const U8Vec3 &v) const {
		return U8Vec3((uint8_t)(x - v.x), (uint8_t)(y - v.y), (uint8_t)(z - v.z));
	}
};

}
