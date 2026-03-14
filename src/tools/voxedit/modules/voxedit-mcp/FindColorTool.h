/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class FindColorTool : public Tool {
public:
	FindColorTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
