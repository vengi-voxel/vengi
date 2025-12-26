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
	NormalPaint = (1 << 3),
	Override = (1 << 4),
	Select = (1 << 5),

	Mask = (Paint | NormalPaint | Place | Erase | Override | Select),
	ExistingVoxelMask = (Paint | NormalPaint | Erase | Override | Select),

	ColorPicker = (1 << 6)
};
CORE_ENUM_BIT_OPERATIONS(ModifierType)

inline bool isModifying(ModifierType type) {
	return type != ModifierType::None && type != ModifierType::Select && type != ModifierType::ColorPicker;
}
