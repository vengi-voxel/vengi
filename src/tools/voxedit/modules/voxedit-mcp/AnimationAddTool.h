/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class AnimationAddTool : public Tool {
public:
	AnimationAddTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
