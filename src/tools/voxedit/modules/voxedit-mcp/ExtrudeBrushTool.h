/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for extruding selected voxels using the ExtrudeBrush
 *
 * Requires a selection (use select_brush first) and a face direction.
 * Extrudes selected voxels outward (positive depth) or carves inward
 * (negative depth) along the specified face normal.
 *
 * @sa @c ExtrudeBrush for the actual brush implementation
 */
class ExtrudeBrushTool : public BrushTool {
public:
	ExtrudeBrushTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
