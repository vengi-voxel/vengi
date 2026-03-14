/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MementoCanRedoTool : public Tool {
public:
	MementoCanRedoTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
