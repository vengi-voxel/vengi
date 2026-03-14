/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoUndoTool : public Tool {
public:
	MementoUndoTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
