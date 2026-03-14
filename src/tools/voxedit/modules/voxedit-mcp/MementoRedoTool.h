/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoRedoTool : public Tool {
public:
	MementoRedoTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
