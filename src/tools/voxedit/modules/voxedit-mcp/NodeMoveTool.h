/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeMoveTool : public Tool {
public:
	NodeMoveTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
