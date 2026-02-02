/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class GetPaletteTool : public Tool {
public:
	GetPaletteTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
