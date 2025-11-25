/**
 * @file
 */

#pragma once

#include "Brush.h"

namespace voxedit {

/**
 * @brief State tracker for line-based brushes to detect when the preview needs updating
 *
 * This helper struct captures the essential state from a BrushContext and provides
 * comparison operators to detect changes. When any tracked parameter changes, the
 * brush knows it needs to regenerate its preview.
 */
struct LineState {
	glm::ivec3 cursorPosition{0}; ///< Current cursor position in voxel space
	glm::ivec3 referencePos{0};	  ///< Reference/start position for the line
	voxel::Voxel cursorVoxel;	  ///< The voxel type that will be placed

	/**
	 * @brief Check if the brush context has changed from the cached state
	 * @return true if any tracked parameter differs from the context
	 */
	bool operator!=(const BrushContext &ctx) {
		return cursorPosition != ctx.cursorPosition || referencePos != ctx.referencePos ||
			   !cursorVoxel.isSame(ctx.cursorVoxel);
	}

	/**
	 * @brief Update the cached state from the brush context
	 */
	void operator=(const BrushContext &ctx) {
		cursorPosition = ctx.cursorPosition;
		referencePos = ctx.referencePos;
		cursorVoxel = ctx.cursorVoxel;
	}
};

} // namespace voxedit
