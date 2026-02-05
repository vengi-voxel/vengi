/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeAddKeyframeTool : public Tool {
public:
	NodeAddKeyframeTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
