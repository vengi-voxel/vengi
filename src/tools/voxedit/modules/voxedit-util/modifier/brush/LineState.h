/**
 * @file
 */

#pragma once

#include "Brush.h"

namespace voxedit {

struct LineState {
	glm::ivec3 cursorPosition{0};
	glm::ivec3 referencePos{0};
	voxel::Voxel cursorVoxel;

	bool operator!=(const BrushContext &ctx) {
		return cursorPosition != ctx.cursorPosition || referencePos != ctx.referencePos ||
			   !cursorVoxel.isSame(ctx.cursorVoxel);
	}

	void operator=(const BrushContext &ctx) {
		cursorPosition = ctx.cursorPosition;
		referencePos = ctx.referencePos;
		cursorVoxel = ctx.cursorVoxel;
	}
};

} // namespace voxedit
