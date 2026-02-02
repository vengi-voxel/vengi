/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeAddModelTool : public Tool {
public:
	NodeAddModelTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
