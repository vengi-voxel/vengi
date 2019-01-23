/**
 * @file
 */

#pragma once

#include "core/Common.h"

enum class ModifierType {
	None = 0,
	Place = 1 << 0,
	Delete = 1 << 1
};
CORE_ENUM_BIT_OPERATIONS(ModifierType)
