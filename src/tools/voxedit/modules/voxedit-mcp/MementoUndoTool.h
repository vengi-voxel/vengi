/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoUndoTool : public Tool {
public:
	MementoUndoTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
