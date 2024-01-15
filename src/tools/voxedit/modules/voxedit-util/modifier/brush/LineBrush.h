/**
 * @file
 */

#pragma once

#include "Brush.h"

namespace voxedit {

class LineBrush : public Brush {
private:
	using Super = Brush;

	struct State {
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

	State _state;

public:
	virtual ~LineBrush() = default;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	voxel::Region calcRegion(const BrushContext &context) const;
};

} // namespace voxedit
