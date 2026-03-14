/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class GetVoxelsTool : public Tool {
public:
	GetVoxelsTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
