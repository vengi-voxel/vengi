/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class GetSceneStateTool : public Tool {
public:
	GetSceneStateTool();
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
