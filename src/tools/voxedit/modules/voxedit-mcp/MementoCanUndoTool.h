/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoCanUndoTool : public Tool {
public:
	MementoCanUndoTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
