/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class FindColorTool : public Tool {
public:
	FindColorTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
