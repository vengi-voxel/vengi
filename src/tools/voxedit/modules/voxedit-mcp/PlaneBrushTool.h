/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for extruding or filling planes using the PlaneBrush
 *
 * @sa @c PlaneBrush for the actual brush implementation
 */
class PlaneBrushTool : public BrushTool {
public:
	PlaneBrushTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
