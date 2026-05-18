/**
 * @file
 */

#pragma once

#include "app/I18NMarkers.h"
#include "core/ArrayLength.h"
#include "ui/IconsLucide.h"

namespace voxedit {

enum ShapeType {
	Box,
	Torus,
	Cylinder,
	Cone,
	Dome,
	Ellipse,
	Circle,

	Max,
	Min = Box,
};

// Command names (shapeaabb, shapetorus, ...) - keep stable for keybindings and scripts.
static constexpr const char *ShapeTypeCmdStr[ShapeType::Max]{"aabb", "torus", "cylinder", "cone", "dome", "ellipse",
															"circle"};

// clang-format off
static constexpr const char *ShapeTypeStr[ShapeType::Max]{
	NC_("ShapeType", "Box"),
	NC_("ShapeType", "Torus"),
	NC_("ShapeType", "Cylinder"),
	NC_("ShapeType", "Cone"),
	NC_("ShapeType", "Dome"),
	NC_("ShapeType", "Ellipse"),
	NC_("ShapeType", "Circle")
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
static_assert(lengthof(ShapeTypeCmdStr) == (int)ShapeType::Max, "Array size mismatch");
static_assert(lengthof(ShapeTypeIcons) == (int)ShapeType::Max, "Array size mismatch");

} // namespace voxedit
