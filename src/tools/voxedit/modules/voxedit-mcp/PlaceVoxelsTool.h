/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class PlaceVoxelsTool : public Tool {
public:
	PlaceVoxelsTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
