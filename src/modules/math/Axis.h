/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "core/String.h"
#include <stdint.h>

namespace math {

enum class Axis : uint8_t { None = 0, X = 1, Y = 2, Z = 4 };

CORE_ENUM_BIT_OPERATIONS(Axis)

inline constexpr int getIndexForAxis(math::Axis axis) {
	if (axis == math::Axis::X) {
		return 0;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 2;
}

/**
 * First remaining axis of a plane perpendicular to @c axis, in X-Y-Z order.
 * X plane -> Y, Y plane -> X, Z plane -> X.
 */
inline constexpr math::Axis getPlaneAxisA(math::Axis axis) {
	return axis == math::Axis::X ? math::Axis::Y : math::Axis::X;
}

/**
 * Second remaining axis of a plane perpendicular to @c axis, in X-Y-Z order.
 * X plane -> Z, Y plane -> Z, Z plane -> Y.
 */
inline constexpr math::Axis getPlaneAxisB(math::Axis axis) {
	return axis == math::Axis::Z ? math::Axis::Y : math::Axis::Z;
}

inline constexpr const char *getCharForAxis(math::Axis axis) {
	switch (axis) {
	case math::Axis::X:
		return "x";
	case math::Axis::Y:
		return "y";
	case math::Axis::Z:
		return "z";
	case math::Axis::None:
		return "none";
	default:
		break;
	}
	return "";
}

math::Axis toAxis(const core::String &axesStr);

} // namespace math
