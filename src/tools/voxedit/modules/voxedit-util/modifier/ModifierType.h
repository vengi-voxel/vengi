/**
 * @file
 */

#pragma once

#include "core/Enum.h"

enum class ModifierType {
	None = 0,
	Place = (1 << 0),
	Single = (1 << 1),
	Delete = (1 << 2),
	Update = (1 << 3),
	Select = (1 << 4),
	ColorPicker = (1 << 5) | Single,
	FillPlane = (1 << 6)
};
CORE_ENUM_BIT_OPERATIONS(ModifierType)
