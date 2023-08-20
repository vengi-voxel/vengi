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

math::Axis toAxis(const core::String &axisStr);

} // namespace math
