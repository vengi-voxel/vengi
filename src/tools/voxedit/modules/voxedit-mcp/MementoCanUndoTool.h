/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoCanUndoTool : public Tool {
public:
	MementoCanUndoTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
