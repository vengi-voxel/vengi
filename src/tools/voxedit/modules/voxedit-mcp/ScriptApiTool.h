/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class ScriptApiTool : public Tool {
public:
	ScriptApiTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
