/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class ScriptApiTool : public Tool {
public:
	ScriptApiTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
