/**
 * @file
 */

#pragma once

#include "math/U8Vec3.h"
#include "math/Vec2f.h"
#include "math/Vec2u.h"
#include "math/Vec3f.h"

namespace math {

template<class T>
struct Vec2Of;
template<>
struct Vec2Of<float> {
	typedef Vec2f Type;
};
template<>
struct Vec2Of<int> {
	typedef Vec2i Type;
};
template<>
struct Vec2Of<uint32_t> {
	typedef Vec2u Type;
};

template<class T>
struct Vec3Of;
template<>
struct Vec3Of<float> {
	typedef Vec3f Type;
};
template<>
struct Vec3Of<int> {
	typedef Vec3i Type;
};
template<>
struct Vec3Of<uint8_t> {
	typedef U8Vec3 Type;
};

}
