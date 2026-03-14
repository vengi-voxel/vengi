/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class GetSceneStateTool : public Tool {
public:
	GetSceneStateTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
