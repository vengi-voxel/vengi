/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class SetPaletteColorTool : public Tool {
public:
	SetPaletteColorTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
