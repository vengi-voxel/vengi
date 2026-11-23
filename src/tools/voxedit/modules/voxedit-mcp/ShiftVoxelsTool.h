/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class ShiftVoxelsTool : public Tool {
public:
	ShiftVoxelsTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
