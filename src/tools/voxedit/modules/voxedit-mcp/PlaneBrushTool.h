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
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
