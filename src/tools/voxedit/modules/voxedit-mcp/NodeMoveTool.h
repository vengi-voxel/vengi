/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeMoveTool : public Tool {
public:
	NodeMoveTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
