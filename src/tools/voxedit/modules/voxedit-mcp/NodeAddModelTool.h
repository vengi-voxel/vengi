/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeAddModelTool : public Tool {
public:
	NodeAddModelTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
