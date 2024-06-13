/**
 * @file
 */

#pragma once

#include "core/Enum.h"

enum class ModifierType {
	None = 0,

	Paint = (1 << 0),
	Place = (1 << 1),
	Erase = (1 << 2),
	Override = (1 << 4),

	Mask = (Paint | Place | Erase | Override),

	Select = (1 << 5),
	ColorPicker = (1 << 6)
};
CORE_ENUM_BIT_OPERATIONS(ModifierType)
