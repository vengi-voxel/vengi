/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "math/Vec3i.h"

namespace voxedit {

/**
 * @brief State tracker for line-based brushes to detect when the preview needs updating
 *
 * This helper struct captures the essential state from a BrushContext and provides
 * comparison operators to detect changes. When any tracked parameter changes, the
 * brush knows it needs to regenerate its preview.
 */
struct LineState {
	math::Vec3i cursorPosition{0}; ///< Current cursor position in voxel space
	math::Vec3i referencePos{0};	  ///< Reference/start position for the line
	math::Vec3i controlPoint{0};	  ///< Quadratic bezier control point in voxel space
	voxel::Voxel cursorVoxel;	  ///< The voxel type that will be placed
	bool bezier = false;

	/**
	 * @brief Check if the brush context has changed from the cached state
	 * @return true if any tracked parameter differs from the context
	 */
	bool hasChanges(const BrushContext &ctx, bool bezierMode, const math::Vec3i &control) const {
		return cursorPosition != ctx.cursorPosition || referencePos != ctx.referencePos ||
			   !cursorVoxel.isSame(ctx.cursorVoxel) || bezier != bezierMode || controlPoint != control;
	}

	/**
	 * @brief Update the cached state from the brush context
	 */
	void update(const BrushContext &ctx, bool bezierMode, const math::Vec3i &control) {
		cursorPosition = ctx.cursorPosition;
		referencePos = ctx.referencePos;
		controlPoint = control;
		cursorVoxel = ctx.cursorVoxel;
		bezier = bezierMode;
	}
};

} // namespace voxedit
