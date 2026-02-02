/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeRenameTool : public Tool {
public:
	NodeRenameTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit