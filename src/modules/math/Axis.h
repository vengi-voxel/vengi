/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include <stdint.h>

namespace math {

enum class Axis : uint8_t {
	None = 0,
	X = 1,
	Y = 2,
	Z = 4
};

CORE_ENUM_BIT_OPERATIONS(Axis)

inline constexpr int getIndexForAxis(math::Axis axis) {
	if (axis == math::Axis::X) {
		return 0;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 2;
}

}
