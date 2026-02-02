/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoRedoTool : public Tool {
public:
	MementoRedoTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
