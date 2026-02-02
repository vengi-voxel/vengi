/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class ScriptCreateTool : public Tool {
public:
	ScriptCreateTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
