/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeRenameTool : public Tool {
public:
	NodeRenameTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit