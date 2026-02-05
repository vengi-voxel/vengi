/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class AnimationAddTool : public Tool {
public:
	AnimationAddTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
