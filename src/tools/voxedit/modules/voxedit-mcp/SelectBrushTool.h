/**
 * @file
 */

#pragma once

#include "BrushTool.h"

namespace voxedit {

/**
 * @brief MCP Tool for selecting voxels using the SelectBrush
 *
 * @sa @c SelectBrush for the actual brush implementation
 */
class SelectBrushTool : public BrushTool {
public:
	SelectBrushTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
