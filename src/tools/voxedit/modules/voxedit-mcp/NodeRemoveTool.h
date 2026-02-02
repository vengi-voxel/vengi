/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeRemoveTool : public Tool {
public:
	NodeRemoveTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
