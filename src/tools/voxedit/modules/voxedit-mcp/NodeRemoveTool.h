/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeRemoveTool : public Tool {
public:
	NodeRemoveTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
