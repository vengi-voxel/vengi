/**
 * @file
 */

#pragma once

#include "BrushGizmo.h"

namespace voxedit {

/**
 * @brief Map a Lua gizmo operation name to BrushGizmoOperation flags
 * @param name Operation string from a script gizmo() table (e.g. "line", "translate")
 */
uint32_t mapBrushGizmoOperation(const char *name);

} // namespace voxedit
