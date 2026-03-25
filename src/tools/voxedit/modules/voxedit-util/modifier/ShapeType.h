/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"
#include "ui/IconsLucide.h"

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

static constexpr const char *ShapeTypeIcons[ShapeType::Max]{
	ICON_LC_BOX,
	ICON_LC_TORUS,
	ICON_LC_CYLINDER,
	ICON_LC_CONE,
	ICON_LC_GLOBE,
	ICON_LC_EGG,
	ICON_LC_CIRCLE_DASHED
};
// clang-format on
static_assert(lengthof(ShapeTypeStr) == (int)ShapeType::Max, "Array size mismatch");
static_assert(lengthof(ShapeTypeIcons) == (int)ShapeType::Max, "Array size mismatch");

} // namespace voxedit
