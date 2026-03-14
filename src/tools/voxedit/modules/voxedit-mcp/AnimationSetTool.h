/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class AnimationSetTool : public Tool {
public:
	AnimationSetTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
