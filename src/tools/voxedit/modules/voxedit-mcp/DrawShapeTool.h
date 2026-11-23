/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

/**
 * @brief Fill or carve a box, sphere, ellipsoid, cylinder or line. Corners are inclusive.
 * color -1 erases. hollow keeps the 6-neighbor shell.
 */
class DrawShapeTool : public Tool {
public:
	DrawShapeTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
