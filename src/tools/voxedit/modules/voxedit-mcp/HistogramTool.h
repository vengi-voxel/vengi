/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

class HistogramTool : public Tool {
public:
	HistogramTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
