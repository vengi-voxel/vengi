/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

/**
 * @brief Stack text rows into voxels. plane xz: each slice is one Y layer, each row is +Z, each
 * character is +X. plane xy: each slice is one Z layer, rows run from the top down.
 */
class DrawSlicesTool : public Tool {
public:
	DrawSlicesTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
