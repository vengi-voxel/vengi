/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class GetPaletteTool : public Tool {
public:
	GetPaletteTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
