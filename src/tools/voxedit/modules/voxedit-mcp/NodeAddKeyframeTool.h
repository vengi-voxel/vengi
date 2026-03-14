/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeAddKeyframeTool : public Tool {
public:
	NodeAddKeyframeTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
