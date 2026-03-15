/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for transforming selected voxels using the TransformBrush
 *
 * Requires a selection (use select_brush first). Supports Move, Shear, Scale,
 * and Rotate transform modes on selected voxels.
 *
 * @sa @c TransformBrush for the actual brush implementation
 */
class TransformBrushTool : public BrushTool {
public:
	TransformBrushTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
