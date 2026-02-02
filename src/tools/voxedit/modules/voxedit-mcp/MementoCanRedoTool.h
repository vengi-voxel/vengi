/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoCanRedoTool : public Tool {
public:
	MementoCanRedoTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
