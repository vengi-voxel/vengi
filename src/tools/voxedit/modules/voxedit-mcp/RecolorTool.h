/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class RecolorTool : public Tool {
public:
	RecolorTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
