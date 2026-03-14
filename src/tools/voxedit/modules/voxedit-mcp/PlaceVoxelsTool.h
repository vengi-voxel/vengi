/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class PlaceVoxelsTool : public Tool {
public:
	PlaceVoxelsTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
