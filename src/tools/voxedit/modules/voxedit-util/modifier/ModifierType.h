/**
 * @file
 */

#pragma once

#include "core/Enum.h"

enum class ModifierType {
	None = 0,
	Place = 1 << 0,
	Delete = 1 << 1,
	Update = 1 << 2,
	Select = 1 << 3,
	ColorPicker = 1 << 4
};
CORE_ENUM_BIT_OPERATIONS(ModifierType)
