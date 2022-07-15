/**
 * @file
 */

#pragma once

#include "core/Enum.h"

enum class ModifierType {
	None = 0,
	// single doesn't span an aabb
	Single = (1 << 0),
	// perform the below actions not on aabbs or single voxels, but on the plane
	Plane = (1 << 1),

	Place = (1 << 2),
	Erase = (1 << 3),
	Paint = (1 << 4),
	Select = (1 << 5),
	ColorPicker = (1 << 6),
	Path = (1 << 7),
	Line = (1 << 8),
};
CORE_ENUM_BIT_OPERATIONS(ModifierType)
