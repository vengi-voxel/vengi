/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include <stdint.h>

namespace voxedit {

enum class SceneModifiedFlags : uint32_t {
	MarkUndo = 1 << 0,
	ResetTrace = 1 << 1,
	Max,

	All = MarkUndo | ResetTrace,
	NoUndo = All & ~MarkUndo,
	NoResetTrace = All & ~ResetTrace
};
CORE_ENUM_BIT_OPERATIONS(SceneModifiedFlags)

} // namespace voxedit
