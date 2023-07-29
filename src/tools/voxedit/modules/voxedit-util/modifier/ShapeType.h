/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"
#include "core/Enum.h"

namespace voxedit {

enum ShapeType {
	AABB,
	Torus,
	Cylinder,
	Cone,
	Dome,
	Ellipse,

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
	"Ellipse"
};
// clang-format on
static_assert(lengthof(ShapeTypeStr) == (int)ShapeType::Max, "Array size mismatch");

} // namespace voxedit
