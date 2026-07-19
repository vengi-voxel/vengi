/**
 * @file
 */

#pragma once

#include "Tool.h"

namespace voxedit {

/**
 * @brief Pixel-perfect or isometric screenshot of a model node or the merged scene via
 * @c voxelutil::renderToImage / @c voxelutil::renderIsometricImage (no GL context required).
 */
class ScreenshotTool : public Tool {
public:
	ScreenshotTool();
	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
