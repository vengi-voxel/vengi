/**
 * @file
 */

#pragma once

#include "core/Common.h"

namespace math {

enum class Axis : uint8_t {
	None = 0,
	X = 1,
	Y = 2,
	Z = 4
};

CORE_ENUM_BIT_OPERATIONS(Axis)

}
