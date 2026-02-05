/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class AnimationSetTool : public Tool {
public:
	AnimationSetTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
