/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"

namespace voxedit {

enum ShapeType {
	AABB,
	Torus,
	Cylinder,
	Cone,
	Dome,
	Ellipse,
	Circle,

	Max,
	Min = AABB,
};

// clang-format off
static constexpr const char *ShapeTypeStr[ShapeType::Max]{
	"AABB",
	"Torus",
	"Cylinder",
	"Cone",
	"Dome",
	"Ellipse",
	"Circle"
};
// clang-format on
static_assert(lengthof(ShapeTypeStr) == (int)ShapeType::Max, "Array size mismatch");

} // namespace voxedit
