/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for drawing straight lines between two points using the LineBrush
 *
 * @sa @c LineBrush for the actual brush implementation
 */
class LineBrushTool : public BrushTool {
public:
	LineBrushTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
