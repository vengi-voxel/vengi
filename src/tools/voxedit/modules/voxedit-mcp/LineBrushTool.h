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
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
