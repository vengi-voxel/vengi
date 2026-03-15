/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for sculpting voxels using the SculptBrush
 *
 * Requires a selection (use select_brush first). Operates on selected voxels
 * with modes like Erode, Grow, Flatten, SmoothAdditive, SmoothErode.
 *
 * @sa @c SculptBrush for the actual brush implementation
 */
class SculptBrushTool : public BrushTool {
public:
	SculptBrushTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
