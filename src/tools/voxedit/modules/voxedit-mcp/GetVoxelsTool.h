/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class GetVoxelsTool : public Tool {
public:
	GetVoxelsTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
