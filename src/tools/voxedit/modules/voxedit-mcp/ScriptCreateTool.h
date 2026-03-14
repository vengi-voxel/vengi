/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class ScriptCreateTool : public Tool {
public:
	ScriptCreateTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
