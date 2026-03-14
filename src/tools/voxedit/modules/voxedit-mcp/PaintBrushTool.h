/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for painting/recoloring existing voxels using the PaintBrush
 *
 * @sa @c PaintBrush for the actual brush implementation
 */
class PaintBrushTool : public BrushTool {
public:
	PaintBrushTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
