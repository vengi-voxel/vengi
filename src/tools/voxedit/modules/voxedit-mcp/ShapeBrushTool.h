/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for creating geometric shapes using the ShapeBrush
 *
 * @sa @c ShapeBrush for the actual brush implementation
 */
class ShapeBrushTool : public BrushTool {
public:
	ShapeBrushTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
