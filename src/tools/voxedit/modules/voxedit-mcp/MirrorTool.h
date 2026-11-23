/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class MirrorTool : public Tool {
public:
	MirrorTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
