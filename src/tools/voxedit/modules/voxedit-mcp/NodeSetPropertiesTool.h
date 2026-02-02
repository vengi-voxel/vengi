/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeSetPropertiesTool : public Tool {
public:
	NodeSetPropertiesTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
