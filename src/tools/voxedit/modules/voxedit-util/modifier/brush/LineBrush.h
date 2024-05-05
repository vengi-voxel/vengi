/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "voxel/Region.h"
#include "LineState.h"

namespace voxedit {

class LineBrush : public Brush {
private:
	using Super = Brush;

	LineState _state;

public:
	LineBrush() : Super(BrushType::Line) {
	}
	virtual ~LineBrush() = default;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	voxel::Region calcRegion(const BrushContext &context) const;
};

} // namespace voxedit
