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
	UpdateRendererRegion = 1 << 2,
	InvalidateNodeCache = 1 << 3,
	Max,

	All = MarkUndo | ResetTrace | UpdateRendererRegion | InvalidateNodeCache,
	NoUndo = All & ~MarkUndo,
	NoResetTrace = All & ~ResetTrace,
	NoRegionUpdate = All & ~UpdateRendererRegion,
	NoNodeCacheInvalidation = All & ~InvalidateNodeCache
};
CORE_ENUM_BIT_OPERATIONS(SceneModifiedFlags)

} // namespace voxedit
