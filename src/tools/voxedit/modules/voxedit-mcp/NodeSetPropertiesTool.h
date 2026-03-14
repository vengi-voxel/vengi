/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class NodeSetPropertiesTool : public Tool {
public:
	NodeSetPropertiesTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
